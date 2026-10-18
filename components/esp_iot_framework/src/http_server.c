/* SPDX-FileCopyrightText: 2026 AmakeSasha
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Path: ./components/esp_iot_framework/src/http_server.c
 * 
 * Copyright 2026 AmakeSasha
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "sdkconfig.h"

#define JSMN_STATIC
#include <jsmn.h>
#include <stdlib.h>
#include <esp_mac.h>
#include <esp_log.h>
#include <esp_flash.h>
#include <esp_timer.h>
#include <esp_ota_ops.h>
#include <esp_chip_info.h>
#include <json_generator.h>
#include <esp_app_format.h>
#include <esp_idf_version.h>
#ifdef CONFIG_EIF_ENABLE_TLS
    #include <esp_https_server.h>
#else
    #include <esp_http_server.h>
#endif
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    #include <esp_clk_tree.h>
#else
    #include <esp_private/esp_clk.h>
#endif

#include "private_include/macros.h"
#include "private_include/internal.h"

#ifdef CONFIG_EIF_ENABLE_TLS
    #define TAG "HTTPS server"
#else
    #define TAG "HTTP server"
#endif

/* --- */
 
#define HTTPD_202 "202 Accepted"
#define HTTPD_401 "401 Unauthorized"
#define HTTPD_409 "409 Conflict"
#define HTTPD_422 "422 Unprocessable Entity"

#define RESP_TYPE_JSON "application/json; charset=UTF-8"
#define RESP_TYPE_TXT  "text/plain; charset=UTF-8"
#define RESP_TYPE_CSS  "text/css; charset=UTF-8"
#define RESP_TYPE_HTML "text/html; charset=UTF-8"
#define RESP_TYPE_JS   "application/javascript; charset=UTF-8"

#ifdef CONFIG_EIF_ENABLE_TLS
    #define EIF_SERVER_TLS_VAL true
#else
    #define EIF_SERVER_TLS_VAL false
#endif

#define SERVER_JSON_OUT_BUF_SIZE 1024
#define SERVER_JSON_IN_BUF_SIZE 256
#define SERVER_JSON_MAX_TOKENS 16U
#define SERVER_OTA_BUFFER_SIZE 4096U

#define HTTP_LOGS_CHUNK_SIZE 512

#define MAX_REASONS_RESET_NUM 14
#define MAC_NUMBERS_LEN 6U
#define MAC_FORMATED_LEN 18U
#define IP_FORMATED_LEN 16U


#define JSON_END_WRITE_AND_SEND(m_jgen, m_req) do {                               \
    int json_len = json_gen_str_end(&(m_jgen));                                   \
    EIF_TRY_CONDITION(((json_len <= 0) || (json_len > SERVER_JSON_OUT_BUF_SIZE)), \
                      ESP_ERR_NO_MEM);                                            \
                                                                                  \
    httpd_resp_set_type((m_req), RESP_TYPE_JSON);                                 \
    httpd_resp_sendstr((m_req), g_server_json_buffer);                            \
} while (false)

#define JSON_START_WRITE(m_jgen) do {                                                          \
    json_gen_str_start(&(m_jgen), g_server_json_buffer, SERVER_JSON_OUT_BUF_SIZE, NULL, NULL); \
} while (false)


#define HTTPD_RESP_SENDSTATUS(m_req, m_status) do {   \
    (void)httpd_resp_set_status((m_req), (m_status)); \
    (void)httpd_resp_sendstr((m_req), "");            \
} while (false)

#define HTTPD_RESP_SENDFILE(m_req, m_type, m_start, m_end) do {           \
    httpd_resp_set_type(req, (m_type));                                   \
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");                  \
    return httpd_resp_send(req, (const char *)m_start, m_end -  m_start); \
} while (false)

#define HTTPD_RESP_NO_CACHE(m_req) do {                                                  \
    httpd_resp_set_hdr((m_req), "Cache-Control", "no-store, no-cache, must-revalidate"); \
} while (false)

#define HTTPD_RESP_SET_CACHE(m_req) do {                                  \
    httpd_resp_set_hdr((m_req), "Cache-Control", "public, max-age=3600"); \
} while (false)



/* Other */

static esp_err_t format_mac_addr_to_buf(
    const uint8_t * const mac, char * const dest_str, size_t dest_size
) {
    esp_err_t ret = ESP_OK;

    EIF_CHECK_NOT_NULL(mac, ESP_ERR_INVALID_ARG);
    EIF_CHECK_NOT_NULL(dest_str, ESP_ERR_INVALID_ARG);
    EIF_TRY_CONDITION(dest_size < MAC_FORMATED_LEN, ESP_ERR_INVALID_ARG);

    int len = snprintf(dest_str, dest_size, "%02X:%02X:%02X:%02X:%02X:%02X",
                       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    if ((len < 0) || (len >= (int)dest_size)) {
        EIF_LOG_E("'mac_formatter' is too long! Max allowed: %u", (unsigned int)dest_size);
        return ESP_ERR_INVALID_SIZE;
    }

cleanup:
    return ret;
}



/* JSON */

/**
 * @brief Static buffer for HTTP server JSON response serialization.
 * 
 * File-scope static allocation limits visibility to this module and protects
 * the FreeRTOS task stack from overhead. Zero dynamic allocation ensures
 * runtime determinism and prevents heap fragmentation on the
 * resource-constrained MCU.
 * 
 * Reusing a single shared buffers minimizes the total RAM footprint across
 * internal serialization routines. Thread safety is guaranteed by design
 * because the underlying ESP-IDF HTTP server executes sequentially within a
 * single FreeRTOS task, eliminating concurrent access. The fixed capacity is
 * sufficient for the framework's internal JSON payloads.
 */
static char g_server_json_buffer[SERVER_JSON_OUT_BUF_SIZE] = {0};

typedef struct {
    char buffer[SERVER_JSON_IN_BUF_SIZE];
    jsmntok_t tokens[SERVER_JSON_MAX_TOKENS];
    int token_count;
} json_parser_ctx_t;

static esp_err_t req_http_parse_json(
    httpd_req_t * const req, json_parser_ctx_t * const ctx
) {
    esp_err_t ret = ESP_OK;

    int received = 0;
    jsmn_parser parser = {0};

    EIF_TRY_CONDITION(req->content_len == 0, ESP_ERR_INVALID_ARG);
    EIF_TRY_CONDITION(req->content_len >= sizeof(ctx->buffer), ESP_ERR_NO_MEM);

    received = httpd_req_recv(req, ctx->buffer, req->content_len);
    EIF_TRY_CONDITION(received <= 0, ESP_ERR_HTTPD_INVALID_REQ);

    ctx->buffer[received] = '\0';
    jsmn_init(&parser);

    int r = jsmn_parse(&parser, ctx->buffer, (size_t)received, ctx->tokens, SERVER_JSON_MAX_TOKENS);
    EIF_TRY_CONDITION((r < 0) || (ctx->tokens[0].type != JSMN_OBJECT), ESP_ERR_INVALID_STATE);
    ctx->token_count = r;

cleanup:
    if (ret != ESP_OK) {
        EIF_LOG_E("Failed to parse HTTP JSON payload, err: %s", esp_err_to_name(ret));
    }
    return ret;
}

static esp_err_t req_json_get_field(
    const json_parser_ctx_t * const ctx, char * const out_value, 
    const char * const field, const size_t min_limit, const size_t max_limit
) {
    esp_err_t ret = ESP_OK;
    int val_idx = -1;
    const size_t field_len = strlen(field);

    for (int i = 1; i < (ctx->token_count - 1); i++) {
        if (ctx->tokens[i].type == JSMN_STRING) {
            const int diff = ctx->tokens[i].end - ctx->tokens[i].start;
            const size_t tok_len = (size_t)diff;

            if (tok_len == field_len) {
                size_t start_idx = (size_t)ctx->tokens[i].start;
                const char *str_ptr = &ctx->buffer[start_idx];
                
                if (strncmp(str_ptr, field, tok_len) == 0) {
                    val_idx = i + 1;
                    break;
                }
            }
        }
    }

    EIF_TRY_CONDITION((val_idx < 0) || (ctx->tokens[val_idx].type != JSMN_STRING && 
                      ctx->tokens[val_idx].type != JSMN_PRIMITIVE), ESP_ERR_INVALID_ARG);

    const int diff = ctx->tokens[val_idx].end - ctx->tokens[val_idx].start;
    const size_t tok_len = (size_t)diff;

    EIF_TRY_CONDITION((tok_len < min_limit) || (tok_len >= max_limit),
                      ESP_ERR_INVALID_ARG);
    
    for (size_t k = 0U; k < tok_len; k++) {
        size_t src_idx = (size_t)ctx->tokens[val_idx].start + k;
        out_value[k] = ctx->buffer[src_idx];
    }
    out_value[tok_len] = '\0';

cleanup:
    if (ret != ESP_OK) {
        EIF_LOG_E("Failed to extract JSON field '%s', err: %s", field, esp_err_to_name(ret));
    }
    return ret;
}
static esp_err_t req_json_get_profile_index(
    const json_parser_ctx_t * const ctx, uint8_t * const index
) {
    esp_err_t ret = ESP_OK;
    char num_buf[4] = {0};

    EIF_TRY(req_json_get_field(ctx, num_buf, "profile_index", 0U, sizeof(num_buf)));

    int val = atoi(num_buf);
    EIF_TRY_CONDITION((val < 0) || (val > (int)CONFIG_EIF_CUSTOM_WIFI_PROFILES_COUNT),
                      ESP_ERR_INVALID_ARG);

    *index = (uint8_t)val;

cleanup:
    if (ret != ESP_OK) {
        EIF_LOG_E("Failed to extract JSON field 'profile_index', value: '%s' err: %s",
                  num_buf, esp_err_to_name(ret));
    }
    return ret;
}



/* Files */

#ifdef CONFIG_EIF_ENABLE_WEB_ADMIN_GUI
    extern const uint8_t api_js_gz_start[] asm("_binary_eif_api_js_gz_start");
    extern const uint8_t api_js_gz_end[]   asm("_binary_eif_api_js_gz_end");
    esp_err_t sendf_api_js_gz(httpd_req_t *req) {
        HTTPD_RESP_SET_CACHE(req);
        HTTPD_RESP_SENDFILE(req, RESP_TYPE_JS, api_js_gz_start, api_js_gz_end);
    }

    extern const uint8_t index_html_gz_start[] asm("_binary_eif_index_html_gz_start");
    extern const uint8_t index_html_gz_end[]   asm("_binary_eif_index_html_gz_end");
    esp_err_t sendf_index_html_gz(httpd_req_t *req) {
        HTTPD_RESP_SET_CACHE(req);
        HTTPD_RESP_SENDFILE(req, RESP_TYPE_HTML, index_html_gz_start, index_html_gz_end);
    }

    #ifdef CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG
        extern const uint8_t logs_html_gz_start[] asm("_binary_eif_logs_html_gz_start");
        extern const uint8_t logs_html_gz_end[]   asm("_binary_eif_logs_html_gz_end");
        esp_err_t sendf_logs_html_gz(httpd_req_t *req) {
            HTTPD_RESP_SET_CACHE(req);
            HTTPD_RESP_SENDFILE(req, RESP_TYPE_HTML, logs_html_gz_start, logs_html_gz_end);
        }
    #endif

    extern const uint8_t network_html_gz_start[] asm("_binary_eif_network_html_gz_start");
    extern const uint8_t network_html_gz_end[]   asm("_binary_eif_network_html_gz_end");
    esp_err_t sendf_network_html_gz(httpd_req_t *req) {
        HTTPD_RESP_SET_CACHE(req);
        HTTPD_RESP_SENDFILE(req, RESP_TYPE_HTML, network_html_gz_start, network_html_gz_end);
    }

    extern const uint8_t style_css_gz_start[] asm("_binary_eif_style_css_gz_start");
    extern const uint8_t style_css_gz_end[]   asm("_binary_eif_style_css_gz_end");
    esp_err_t sendf_style_css_gz(httpd_req_t *req) {
        HTTPD_RESP_SET_CACHE(req);
        HTTPD_RESP_SENDFILE(req, RESP_TYPE_CSS, style_css_gz_start, style_css_gz_end);
    }

    extern const uint8_t system_html_gz_start[] asm("_binary_eif_system_html_gz_start");
    extern const uint8_t system_html_gz_end[]   asm("_binary_eif_system_html_gz_end");
    esp_err_t sendf_system_html_gz(httpd_req_t *req) {
        HTTPD_RESP_SET_CACHE(req);
        HTTPD_RESP_SENDFILE(req, RESP_TYPE_HTML, system_html_gz_start, system_html_gz_end);
    }
#endif



/* WiFi */

static esp_err_t h_wifi_list_json(httpd_req_t * const req) {
    esp_err_t ret = ESP_OK;

    HTTPD_RESP_NO_CACHE(req);

    json_gen_str_t jgen = {0};
    wifi_ap_record_t info = {0};
    char ssid[NVS_WIFI_SSID_MAX_LEN + 1U] = {0};
    char pass[NVS_WIFI_PASS_MAX_LEN + 1U] = {0};

    EIF_TRY(esp_wifi_sta_get_ap_info(&info));

    JSON_START_WRITE(jgen);
    /* { */
    (void)json_gen_start_object(&jgen);
    /*   "current_profile_index": 2, */
    (void)json_gen_obj_set_int(&jgen, "current_profile_index", wifi_get_current_profile_index());
    /*   "rssi_now_profile": -45, */
    (void)json_gen_obj_set_int(&jgen, "rssi_now_profile", info.rssi);
    /*   "profiles": [ */
    (void)json_gen_push_array(&jgen, "profiles");

    for (uint16_t idx = 0U; idx <= CONFIG_EIF_CUSTOM_WIFI_PROFILES_COUNT; idx++) {
        EIF_TRY(nvs_wifi_profile_load(idx, &ssid, &pass));
    /*     { */
        (void)json_gen_start_object(&jgen);
    /*       "ssid": "ESP32_SETUP", */
        (void)json_gen_obj_set_string(&jgen, "ssid", ssid);
        if (idx == 0U) {
    /*       "password": "12345678" */
            (void)json_gen_obj_set_string(&jgen, "password", pass);
        }
    /*     }, */
        (void)json_gen_end_object(&jgen);
    }

    /*   ] */
    (void)json_gen_pop_array(&jgen);
    /* } */
    (void)json_gen_end_object(&jgen);
    JSON_END_WRITE_AND_SEND(jgen, req);

cleanup:
    if (ret != ESP_OK) {
        HTTPD_RESP_SENDSTATUS(req, HTTPD_500);
    }

    return ret;
}

static esp_err_t h_wifi_info_json(httpd_req_t * const req) {
    esp_err_t ret = ESP_OK;

    HTTPD_RESP_NO_CACHE(req);

    #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        wifi_protocol_type_t proto = 0U;
    #else
        uint8_t proto = 0U;
    #endif

    json_gen_str_t jgen = {0};
    const char * band_str = "";
    const char * proto_str = "";
    wifi_ap_record_t ap_info = {0};
    char ip[IP_FORMATED_LEN] = {0};
    char gw[IP_FORMATED_LEN] = {0};
    const char * auth_mode_str = "";
    esp_netif_ip_info_t ip_info = {0};
    const char * lwip_hostname = NULL;
    char netmask[IP_FORMATED_LEN] = {0};
    char mac_str[MAC_FORMATED_LEN] = {0};
    char bssid_str[MAC_FORMATED_LEN] = {0};
    uint8_t own_mac[MAC_NUMBERS_LEN] = {0};
    const eif_config_t * const cfg = eif_config_get();

    esp_netif_t * netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");

    EIF_CHECK_NOT_NULL(netif, ESP_ERR_INVALID_STATE);
    EIF_TRY(esp_wifi_sta_get_ap_info(&ap_info));
    EIF_TRY(esp_netif_get_ip_info(netif, &ip_info));
    EIF_TRY_CONDITION((ip_info.ip.addr == 0U), ESP_ERR_INVALID_STATE);
    EIF_TRY(esp_wifi_get_protocol(WIFI_IF_STA, &proto));

    EIF_TRY(esp_read_mac(own_mac, ESP_MAC_WIFI_STA));
    EIF_TRY(format_mac_addr_to_buf(own_mac, mac_str, sizeof(mac_str)));
    EIF_TRY(format_mac_addr_to_buf(ap_info.bssid, bssid_str, sizeof(bssid_str)));
    EIF_TRY(esp_netif_get_hostname(netif, &lwip_hostname));

    switch (ap_info.authmode) {
        case WIFI_AUTH_OPEN:
            auth_mode_str = "OPEN";
            break;
        case WIFI_AUTH_WEP:
            auth_mode_str = "WEP";
            break;
        case WIFI_AUTH_WPA_PSK:
            auth_mode_str = "WPA_PSK";
            break;
        case WIFI_AUTH_WPA2_PSK:
            auth_mode_str = "WPA2_PSK";
            break;
        case WIFI_AUTH_WPA_WPA2_PSK:
            auth_mode_str = "WPA_WPA2_PSK";
            break;
        case WIFI_AUTH_WPA2_ENTERPRISE:
            auth_mode_str = "WPA2_ENTERPRISE";
            break;
        case WIFI_AUTH_WPA3_PSK:
            auth_mode_str = "WPA3_PSK";
            break;
        case WIFI_AUTH_WPA2_WPA3_PSK:
            auth_mode_str = "WPA2_WPA3_PSK";
            break;
        case WIFI_AUTH_WAPI_PSK:
            auth_mode_str = "WAPI_PSK";
            break;
        case WIFI_AUTH_WPA3_ENT_192:
            auth_mode_str = "WPA3_ENT_192";
            break;
        case WIFI_AUTH_MAX:
            auth_mode_str = "UNKNOWN";
            break;
        default:
            auth_mode_str = "UNKNOWN";
            break;
    }

    #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        if ((proto & WIFI_PROTOCOL_11AX) != 0U) {
            proto_str = "802.11ax (Wi-Fi 6)";
        } else if ((proto & WIFI_PROTOCOL_11N) != 0U) {
            proto_str = "802.11n (Wi-Fi 4)";
        } else if ((proto & WIFI_PROTOCOL_11G) != 0U) {
            proto_str = "802.11g (Wi-Fi 3)";
        } else {
            proto_str = "802.11b (Wi-Fi 1)";
        }
    #else
        if ((proto & (uint8_t)WIFI_PROTOCOL_11N) != 0U) {
            proto_str = "802.11n (Wi-Fi 4)";
        } else if ((proto & (uint8_t)WIFI_PROTOCOL_11G) != 0U) {
            proto_str = "802.11g (Wi-Fi 3)";
        } else {
            proto_str = "802.11b (Wi-Fi 1)";
        }
    #endif

    if (ap_info.primary >= 32U) {
        band_str = "5 GHz";
    } else {
        band_str = "2.4 GHz";
    }

    (void)esp_ip4addr_ntoa(&ip_info.ip, ip, sizeof(ip));
    (void)esp_ip4addr_ntoa(&ip_info.gw, gw, sizeof(gw));
    (void)esp_ip4addr_ntoa(&ip_info.netmask, netmask, sizeof(netmask));

    JSON_START_WRITE(jgen);
    /* { */
    (void)json_gen_start_object(&jgen);
    /*   "ssid": "MyNetwork", */
    (void)json_gen_obj_set_string(&jgen, "ssid",          (char*)ap_info.ssid);
    /*   "rssi": -65, */
    (void)json_gen_obj_set_int(&jgen,    "rssi",          ap_info.rssi);
    /*   "channel": 6, */
    (void)json_gen_obj_set_int(&jgen,    "channel",       ap_info.primary);
    /*   "auth_mode": "WPA2_PSK", */
    (void)json_gen_obj_set_string(&jgen, "auth_mode",     auth_mode_str);
    /*   "bssid": "00:11:22:33:44:55", */
    (void)json_gen_obj_set_string(&jgen, "bssid",         bssid_str);
    /*   "ip": "192.168.1.45", */
    (void)json_gen_obj_set_string(&jgen, "ip",            ip);
    /*   "gateway": "192.168.1.1", */
    (void)json_gen_obj_set_string(&jgen, "gateway",       gw);
    /*   "netmask": "255.255.255.0", */
    (void)json_gen_obj_set_string(&jgen, "netmask",       netmask);
    /*   "mac": "AA:BB:CC:DD:EE:FF" */
    (void)json_gen_obj_set_string(&jgen, "mac",           mac_str);
    /*   "proto": "802.11n (Wi-Fi 4)", */
    (void)json_gen_obj_set_string(&jgen, "proto",         proto_str);
    /*   "band": "2.4 GHz", */
    (void)json_gen_obj_set_string(&jgen, "band",          band_str);
    /*   "lwip_hostname": "device-aabbcc", */
    (void)json_gen_obj_set_string(&jgen, "lwip_hostname", lwip_hostname);
    /*   "mdns_hostname": "device-aabbcc", */
    (void)json_gen_obj_set_string(&jgen, "mdns_hostname", cfg->mdns_hostname);
    /* } */
    (void)json_gen_end_object(&jgen);
    JSON_END_WRITE_AND_SEND(jgen, req);

cleanup:
    if (ret != ESP_OK) {
        HTTPD_RESP_SENDSTATUS(req, HTTPD_500);
    }
    
    return ret;
}

static esp_err_t h_wifi_update_do(httpd_req_t * const req) {
    esp_err_t ret = ESP_OK;

    HTTPD_RESP_NO_CACHE(req);

    uint8_t index = 0;
    json_parser_ctx_t parser_ctx = {0};
    char ssid[NVS_WIFI_SSID_MAX_LEN + 1U] = {0};
    char pass[NVS_WIFI_PASS_MAX_LEN + 1U] = {0};

    EIF_TRY(req_http_parse_json(req, &parser_ctx));
    EIF_TRY(req_json_get_profile_index(&parser_ctx, &index));
    EIF_TRY(req_json_get_field(&parser_ctx, ssid, "ssid",
                               NVS_WIFI_SSID_MIN_LEN, NVS_WIFI_SSID_MAX_LEN));
    EIF_TRY(req_json_get_field(&parser_ctx, pass, "password",
                               NVS_WIFI_PASS_MIN_LEN, NVS_WIFI_PASS_MAX_LEN));
    
    if (nvs_wifi_profile_save(index, &ssid, &pass) == ESP_OK) {
        HTTPD_RESP_SENDSTATUS(req, HTTPD_204);
    } else {
        HTTPD_RESP_SENDSTATUS(req, HTTPD_500);
    }

cleanup:
    if (ret == ESP_ERR_NO_MEM) {
        HTTPD_RESP_SENDSTATUS(req, HTTPD_500);
    } else if (ret != ESP_OK) {
        HTTPD_RESP_SENDSTATUS(req, HTTPD_400);
    }
    return ret;
}

static esp_err_t h_wifi_clear_do(httpd_req_t * const req) {
    esp_err_t ret = ESP_OK;
    
    HTTPD_RESP_NO_CACHE(req);

    uint8_t index = 0;
    json_parser_ctx_t parser_ctx = {0};
    const char empty_ssid[NVS_WIFI_SSID_MAX_LEN + 1U] = {0};
    const char empty_pass[NVS_WIFI_PASS_MAX_LEN + 1U] = {0};

    EIF_TRY(req_http_parse_json(req, &parser_ctx));
    EIF_TRY(req_json_get_profile_index(&parser_ctx, &index));
    
    if (nvs_wifi_profile_save(index, &empty_ssid, &empty_pass) == ESP_OK) {
        HTTPD_RESP_SENDSTATUS(req, HTTPD_204);
    } else {
        HTTPD_RESP_SENDSTATUS(req, HTTPD_500);
    }

cleanup:
    if (ret != ESP_OK) {    
        HTTPD_RESP_SENDSTATUS(req, HTTPD_400);
    }
    return ret;
}

static esp_err_t h_wifi_test_do(httpd_req_t * const req) {
    esp_err_t ret = ESP_OK;
    
    HTTPD_RESP_NO_CACHE(req);

    uint8_t index = 0;
    json_parser_ctx_t parser_ctx = {0};
    char ssid[NVS_WIFI_SSID_MAX_LEN + 1U] = {0};
    char pass[NVS_WIFI_PASS_MAX_LEN + 1U] = {0};

    EIF_TRY(req_http_parse_json(req, &parser_ctx));
    EIF_TRY(req_json_get_profile_index(&parser_ctx, &index));

    EIF_TRY(nvs_wifi_profile_load(index, &ssid, &pass));
    if (wifi_is_ssid_available(ssid)) {
        EIF_LOG_I("SSID '%s' detected", ssid);
        HTTPD_RESP_SENDSTATUS(req, HTTPD_204);
    } else {
        EIF_LOG_W("SSID '%s' not found", ssid);
        HTTPD_RESP_SENDSTATUS(req, HTTPD_422);
    }

cleanup:
    if (ret != ESP_OK) {
        HTTPD_RESP_SENDSTATUS(req, HTTPD_422);
    } 
    return ret;
}



/* TLS */

#ifdef CONFIG_EIF_ENABLE_TLS
    static esp_err_t h_tls_recreate_do(httpd_req_t * const req) {
        esp_err_t ret = ESP_OK;
    
        HTTPD_RESP_NO_CACHE(req);

        EIF_TRY(nvs_tls_creds_remove());
        HTTPD_RESP_SENDSTATUS(req, HTTPD_202);
        sys_launch_task_reboot();

    cleanup:
        if (ret != ESP_OK) {
            HTTPD_RESP_SENDSTATUS(req, HTTPD_500);
        }
        return ret;
    }
#endif



/* System */

static esp_err_t h_sys_info_json(httpd_req_t * const req) {
    static const char* reasons[MAX_REASONS_RESET_NUM] = {
        /*  0 */ "",
        /*  1 */ "Power-On reset",
        /*  2 */ "External pin reset",
        /*  3 */ "Software reset",
        /*  4 */ "System panic reset",
        /*  5 */ "Watchdog timer reset",
        /*  6 */ "Task watchdog reset",
        /*  7 */ "Interrupt watchdog reset",
        /*  8 */ "Deep sleep wake-up reset",
        /*  9 */ "SDIO reset",
        /* 10 */ "USB reset",
        /* 11 */ "JTAG reset",
        /* 12 */ "RTC system reset",
        /* 13 */ "RTC CPU reset",
    };

    esp_err_t ret = ESP_OK;
    
    HTTPD_RESP_NO_CACHE(req);
    
    uint32_t cpu_freq = 0;
    uint32_t flash_size = 0;
    json_gen_str_t jgen = {0};
    esp_chip_info_t chip = {0};
    const char *reset_reason_str = "";
    char mac_str[MAC_FORMATED_LEN] = {0};
    uint8_t own_mac[MAC_NUMBERS_LEN] = {0};
    int reset_reason_int = esp_rom_get_reset_reason(0);
    int largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
    
    #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        uint32_t freq_hz = 0;
        if (esp_clk_tree_src_get_freq_hz(
            SOC_MOD_CLK_CPU, ESP_CLK_TREE_SRC_FREQ_FOCUS_MAX, &freq_hz
        ) == ESP_OK) {
            cpu_freq = freq_hz / 1000000;
        } else {
            cpu_freq = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;
        }
    #else
        cpu_freq = esp_clk_cpu_freq() / 1000000;
    #endif

    esp_chip_info(&chip);
    EIF_TRY(esp_flash_get_size(NULL, &flash_size));
    EIF_TRY(esp_read_mac(own_mac, ESP_MAC_WIFI_STA));
    EIF_TRY(format_mac_addr_to_buf(own_mac, mac_str, sizeof(mac_str)));

    if ((reset_reason_int >= 1) && (reset_reason_int < MAX_REASONS_RESET_NUM)) {
        reset_reason_str = reasons[reset_reason_int];
    } else {
        ret = ESP_ERR_INVALID_STATE;
        goto cleanup;
    }

    JSON_START_WRITE(jgen);
    /* { */
    (void)json_gen_start_object(&jgen);
    /*   "heap_free": 123456, */
    (void)json_gen_obj_set_int(&jgen, "heap_free", esp_get_free_heap_size());
    /*   "heap_min": 120000, */
    (void)json_gen_obj_set_int(&jgen, "heap_min", esp_get_minimum_free_heap_size());
    /*   "largest_block": 80000, */
    (void)json_gen_obj_set_int(&jgen, "largest_block", largest_block);
    /*   "uptime": 3600, */
    (void)json_gen_obj_set_int(&jgen, "uptime", esp_timer_get_time() / 1000000ULL);
    /*   "cores": 2, */
    (void)json_gen_obj_set_int(&jgen, "cores", chip.cores);
    /*   "chip_rev": 3, */
    (void)json_gen_obj_set_int(&jgen, "chip_rev", chip.revision);
    /*   "flash_size": 4, */
    (void)json_gen_obj_set_int(&jgen, "flash_size", flash_size / (1024U * 1024U));
    /*   "cpu_freq": 240, */
    (void)json_gen_obj_set_int(&jgen, "cpu_freq", cpu_freq);
    /*   "reset_reason": 1, */
    (void)json_gen_obj_set_int(&jgen, "reset_reason", reset_reason_int);
    /*   "reset_reason_str": "Power-On Reset", */
    (void)json_gen_obj_set_string(&jgen, "reset_reason_str", reset_reason_str);
    /*   "chip_model": "ESP32", */
    (void)json_gen_obj_set_string(&jgen, "chip_model", CONFIG_IDF_TARGET);
    /*   "features": { */
    (void)json_gen_push_object(&jgen, "features");
    /*     "has_wifi": true, */
    (void)json_gen_obj_set_bool(&jgen, "has_wifi", (chip.features & CHIP_FEATURE_WIFI_BGN));
    /*     "has_bluetooth": true, */
    (void)json_gen_obj_set_bool(&jgen, "has_bluetooth", (chip.features & CHIP_FEATURE_BT));
    /*     "has_ble": true */
    (void)json_gen_obj_set_bool(&jgen, "has_ble", (chip.features & CHIP_FEATURE_BLE));
    /*   }, */
    (void)json_gen_pop_object(&jgen);
    /*   "mac": "AA:BB:CC:DD:EE:FF" */
    (void)json_gen_obj_set_string(&jgen, "mac", mac_str);
    /* } */
    (void)json_gen_end_object(&jgen);
    JSON_END_WRITE_AND_SEND(jgen, req);

cleanup:
    if (ret != ESP_OK) {
        HTTPD_RESP_SENDSTATUS(req, HTTPD_500);
    }
    return ret;
}

static esp_err_t h_sys_reboot_do(httpd_req_t * const req) {
    esp_err_t ret = ESP_OK;

    HTTPD_RESP_NO_CACHE(req);

    HTTPD_RESP_SENDSTATUS(req, HTTPD_202);
    sys_launch_task_reboot();

    /* Cleanup */
    return ret;
}

#ifdef CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG
    static esp_err_t h_sys_logs_txt(httpd_req_t *req) {
        esp_err_t ret = ESP_OK;
    
        HTTPD_RESP_NO_CACHE(req);

        char tx_buffer[HTTP_LOGS_CHUNK_SIZE] = {0};
        
        (void)httpd_resp_set_type(req, RESP_TYPE_TXT);
        size_t bytes_read = core_log_pop_chunk(tx_buffer, HTTP_LOGS_CHUNK_SIZE);

        while (bytes_read > 0U) {
            EIF_TRY(httpd_resp_send_chunk(req, tx_buffer, bytes_read));
            bytes_read = core_log_pop_chunk(tx_buffer, HTTP_LOGS_CHUNK_SIZE);
        }

        (void)httpd_resp_send_chunk(req, NULL, 0U);

    cleanup:
        return ret;
    }
#endif



/* OTA */

static esp_err_t h_ota_info_json(httpd_req_t * const req) {
    esp_err_t ret = ESP_OK;
    
    HTTPD_RESP_NO_CACHE(req);

    char sha_str[65] = {0};
    json_gen_str_t jgen = {0};
    const char * status_str = "unknown";
    esp_ota_img_states_t ota_state = ESP_OTA_IMG_NEW;

    #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        const esp_app_desc_t *app = esp_app_get_description();
    #else
        const esp_app_desc_t *app = esp_ota_get_app_description();
    #endif
    const esp_partition_t *running = esp_ota_get_running_partition();

    #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        esp_app_get_elf_sha256(sha_str, sizeof(sha_str));
    #else
        esp_ota_get_app_elf_sha256(sha_str, sizeof(sha_str));
    #endif

    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        switch (ota_state) {
            case ESP_OTA_IMG_NEW:
                status_str = "new";
                break;
            case ESP_OTA_IMG_PENDING_VERIFY:
                status_str = "pending_verify";
                break;
            case ESP_OTA_IMG_VALID:
                status_str = "valid";
                break;
            case ESP_OTA_IMG_INVALID:
                status_str = "invalid";
                break;
            case ESP_OTA_IMG_ABORTED:
                status_str = "aborted";
                break;
            default:
                status_str = "undefined";
                break;
        }
    } else {
        status_str = "factory";
    }

    JSON_START_WRITE(jgen);
    /* { */
    (void)json_gen_start_object(&jgen);
    /*   "project": "my_iot_project", */
    (void)json_gen_obj_set_string(&jgen, "project", app->project_name);
    /*   "version": "1.0.0", */
    (void)json_gen_obj_set_string(&jgen, "version", app->version);
    /*   "elf_sha256": "a1b2c3d4e5f6...", */
    (void)json_gen_obj_set_string(&jgen, "elf_sha256", sha_str);
    /*   "build_date": "Jan 1 2026", */
    (void)json_gen_obj_set_string(&jgen, "build_date", app->date);
    /*   "build_time": "12:00:00", */
    (void)json_gen_obj_set_string(&jgen, "build_time", app->time);
    /*   "idf_version": "v4.4.6", */
    (void)json_gen_obj_set_string(&jgen, "idf_version", app->idf_ver);
    /*   "compiler": "gcc 8.4.0", */
    (void)json_gen_obj_set_string(&jgen, "compiler", __VERSION__);
    /*   "target": "esp32", */
    (void)json_gen_obj_set_string(&jgen, "target", CONFIG_IDF_TARGET);
    /*   "partition": "factory", */
    (void)json_gen_obj_set_string(&jgen, "partition", running->label);
    /*   "ota_status": "valid" */
    (void)json_gen_obj_set_string(&jgen, "ota_status", status_str);
    /* } */
    (void)json_gen_end_object(&jgen);
    JSON_END_WRITE_AND_SEND(jgen, req);

cleanup:
    if (ret != ESP_OK) {
        HTTPD_RESP_SENDSTATUS(req, HTTPD_500);
    }
    return ret;
}

static esp_err_t h_ota_update_do(httpd_req_t * const req) {
    static bool is_ota_busy = false;

    esp_err_t ret = ESP_OK;
    
    HTTPD_RESP_NO_CACHE(req);

    char * buf = NULL;
    esp_ota_handle_t update_handle = 0;
    size_t remaining = req->content_len;
    const esp_partition_t * update_partition = NULL;

    if (is_ota_busy) {
        EIF_LOG_W("OTA Update already in progress. Rejecting request.");
        ret = ESP_ERR_INVALID_STATE;
    } else {
        is_ota_busy = true;
    }

    if (ret == ESP_OK) {
        update_partition = esp_ota_get_next_update_partition(NULL);
        EIF_TRY_CONDITION(update_partition == NULL, ESP_FAIL);
    }

    if (ret == ESP_OK) {
        buf = (char *)pvPortMalloc(SERVER_OTA_BUFFER_SIZE);
        EIF_CHECK_NOT_NULL(buf, ESP_ERR_NO_MEM);
    }
       
    while (remaining > 0U) {
        const bool flag = remaining < (size_t)SERVER_OTA_BUFFER_SIZE;
        const int chunk_size = flag ? (int)remaining : SERVER_OTA_BUFFER_SIZE;
        const int received = httpd_req_recv(req, buf, chunk_size);

        if (received <= 0) {
            if (received != HTTPD_SOCK_ERR_TIMEOUT) {
                ret = ESP_FAIL;
                goto cleanup;
            }
        } else {
            if (update_handle == 0) {
                EIF_TRY_CONDITION((uint8_t)buf[0] != 0xE9U, ESP_ERR_INVALID_ARG);
                EIF_TRY(esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle));
            }

            EIF_TRY(esp_ota_write(update_handle, (const void *)buf, received));
            remaining -= (size_t)received;
        }
    }

    EIF_TRY(esp_ota_end(update_handle));
    EIF_TRY(esp_ota_set_boot_partition(update_partition));

    if (buf != NULL) {
        vPortFree(buf);
    }

    update_handle = 0;
    HTTPD_RESP_SENDSTATUS(req, HTTPD_204);
    sys_launch_task_reboot();

    /* Cleanup */
cleanup:
    if (ret != ESP_OK) {
        esp_ota_abort(update_handle);

        if (ret == ESP_ERR_INVALID_STATE) {
            HTTPD_RESP_SENDSTATUS(req, HTTPD_409);
        } else if (ret == ESP_ERR_INVALID_ARG) {
            HTTPD_RESP_SENDSTATUS(req, HTTPD_400);
            is_ota_busy = false;
        } else {
            HTTPD_RESP_SENDSTATUS(req, HTTPD_500);
            is_ota_busy = false;
        }
    }
    return ret;
}

static esp_err_t h_ota_confirm_do(httpd_req_t * const req) {
    HTTPD_RESP_NO_CACHE(req);

    if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) {
        HTTPD_RESP_SENDSTATUS(req, HTTPD_202);
    } else {
        HTTPD_RESP_SENDSTATUS(req, HTTPD_500);
    }
    
    /* Cleanup */
    return ESP_OK;
}

static esp_err_t h_ota_rollback_do(httpd_req_t * const req) {
    const eif_config_t * const cfg = eif_config_get();

    HTTPD_RESP_NO_CACHE(req);

    HTTPD_RESP_SENDSTATUS(req, HTTPD_202);

    vTaskDelay(pdMS_TO_TICKS(100));
    if (cfg->sys_handler_reboot != NULL) {
        cfg->sys_handler_reboot();
    }
    sys_launch_task_reboot();
    
    /* Cleanup */
    return ESP_OK;
}



/* Basic Auth */

static esp_err_t h_apass_update_do(httpd_req_t * const req) {
    esp_err_t ret = ESP_OK;
    
    HTTPD_RESP_NO_CACHE(req);

    json_parser_ctx_t parser_ctx = {0};
    char pass[NVS_BASIC_AUTH_PASS_MAX_LEN + 1U] = {0};

    EIF_TRY(req_http_parse_json(req, &parser_ctx));
    EIF_TRY(req_json_get_field(&parser_ctx, pass, "password",
                               NVS_BASIC_AUTH_PASS_MIN_LEN, NVS_BASIC_AUTH_PASS_MAX_LEN));

    if (nvs_basic_auth_line_save(&pass) == ESP_OK) {
        HTTPD_RESP_SENDSTATUS(req, HTTPD_204);
    } else {
        HTTPD_RESP_SENDSTATUS(req, HTTPD_500);
    }

cleanup:
    if (ret != ESP_OK) {
        HTTPD_RESP_SENDSTATUS(req, HTTPD_400);
    }
    return ret;
}



/* Core */

static httpd_handle_t g_server_handle = NULL;
static const char * method_to_str(const httpd_method_t method) {
    const char *result = "";

    switch (method) {
        case HTTP_GET:    
            result = "GET";    
            break;
        case HTTP_POST:   
            result = "POST";   
            break;
        case HTTP_PUT:    
            result = "PUT";    
            break;
        case HTTP_DELETE: 
            result = "DELETE"; 
            break;
        case HTTP_HEAD:   
            result = "HEAD";   
            break;
        case HTTP_OPTIONS:
            result = "OPTIONS";
            break;
        case HTTP_PATCH:  
            result = "PATCH";  
            break;
        case HTTP_TRACE:  
            result = "TRACE";  
            break;
        case HTTP_CONNECT:
            result = "CONNECT";
            break;
        default:          
            result = "UNKNOWN";
            break;
    }

    return result;
}

static esp_err_t req_check_auth_admin(httpd_req_t *req) {
    esp_err_t ret = ESP_OK;

    /* @note +8 bytes to the buffer avoids password truncation. Example (imagine that
     *       'AUTH_LINE_MAX_LEN' = 18):
     *         Header: Basic YWRtaW46MTIzc2Q=  (22 bytes)
     *         NVS:    Basic YWRtaW46MTIz      (18 bytes)
     *       In this case, without +8 bytes, the data from the header will be truncated to the
     *       length of the NVS and the comparison will be successful. */
    char buf[NVS_BASIC_AUTH_LINE_MAX_LEN + 8U] = {0};
    char current_pw[NVS_BASIC_AUTH_LINE_MAX_LEN + 1U] = {0};

    ret = httpd_req_get_hdr_value_str(req, "Authorization", buf, sizeof(buf));
    if ((ret != ESP_OK) && (ret != ESP_ERR_NOT_FOUND)) {
        HTTPD_RESP_NO_CACHE(req);
        HTTPD_RESP_SENDSTATUS(req, HTTPD_400);
    }

    if (ret == ESP_OK) {
        ret = nvs_basic_auth_line_load(&current_pw);
        if ((ret != ESP_OK) && (ret != ESP_ERR_NOT_FOUND)) {
            HTTPD_RESP_NO_CACHE(req);
            HTTPD_RESP_SENDSTATUS(req, HTTPD_500);
        }
    }

    if ((ret == ESP_ERR_NOT_FOUND) || (strcmp(buf, current_pw) != 0)) {
        EIF_LOG_W("UNAUTHORIZED");
        httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"web_basic_auth\"");
        HTTPD_RESP_SENDSTATUS(req, HTTPD_401);
    }

    return ret;
}

static esp_err_t middleware_universal(httpd_req_t * const req) {
    esp_err_t ret = ESP_OK;
    uint32_t start_time = esp_log_timestamp();

    EIF_LOG_I("%s %s", method_to_str(req->method), req->uri);

    const httpd_uri_t* orig = (const httpd_uri_t*)req->user_ctx;
    esp_err_t (*real_handler)(httpd_req_t *) = (esp_err_t (*)(httpd_req_t *))orig->handler;

    if (req_check_auth_admin(req) == ESP_OK) {
        req->user_ctx = orig->user_ctx;
        ret = real_handler(req);
        EIF_LOG_I("Result(%u): %s",
                  esp_log_timestamp() - (size_t)start_time, esp_err_to_name(ret));
    }
    
    /* Cleanup */
    return ret;
}

static esp_err_t register_uris_with_middleware(
    const httpd_uri_t * const routes, size_t route_count, bool need_basic_auth
) {
    esp_err_t ret = ESP_OK;

    if (route_count == 0U) {
        EIF_LOG_W("No handlers to register");
    } else {
        EIF_CHECK_NOT_NULL(routes, ESP_ERR_INVALID_ARG);

        for (size_t i = 0; i < route_count; i++) {
            httpd_uri_t route_to_reg = routes[i];
           
            if (need_basic_auth) {
                route_to_reg.user_ctx = (void *)&routes[i];
                route_to_reg.handler = middleware_universal;
            }

            if ((ret = httpd_register_uri_handler(g_server_handle, &route_to_reg)) != ESP_OK) {
                EIF_LOG_E("Failed to register URI: %s, err: %s", route_to_reg.uri,
                          esp_err_to_name(ret));
                goto cleanup;
            }
               
            EIF_LOG_I("Registered: %-6s %s", method_to_str(route_to_reg.method), route_to_reg.uri);
        }
    }

cleanup:
    return ret;
}

static const httpd_uri_t handlers[] = {
    #ifdef CONFIG_EIF_ENABLE_WEB_ADMIN_GUI
        /* ------------------------- Files ------------------------ */
        #ifdef CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG
            {"/_/files/logs.html", HTTP_GET, sendf_logs_html_gz,   NULL},
        #endif
        {"/_/",                   HTTP_GET, sendf_index_html_gz,   NULL},
        {"/_/files/api.js",       HTTP_GET, sendf_api_js_gz,       NULL},
        {"/_/files/style.css",    HTTP_GET, sendf_style_css_gz,    NULL},
        {"/_/files/system.html",  HTTP_GET, sendf_system_html_gz,  NULL},
        {"/_/files/network.html", HTTP_GET, sendf_network_html_gz, NULL},
    #endif

    /* ----------------------- WiFi API ---------------------- */
    /* Get parameters of all WiFi profiles */
    {"/_/wifi/list",   HTTP_GET,  h_wifi_list_json, NULL},
    /* Get Wi-Fi and network information */
    {"/_/wifi/info",   HTTP_GET,  h_wifi_info_json, NULL},
    /* Update WiFi profile under index X */
    {"/_/wifi/update", HTTP_POST, h_wifi_update_do, NULL},
    /* Clear WiFi profile under index X */
    {"/_/wifi/clear",  HTTP_POST, h_wifi_clear_do,  NULL},
    /* Check WiFi availability using parameters under index X */
    {"/_/wifi/test",  HTTP_POST, h_wifi_test_do,  NULL},

    #ifdef CONFIG_EIF_ENABLE_TLS
        /* ----------------------- TLS API ---------------- ---- */
        /* Generate new TLS keys and certificate */
        {"/_/tls/recreate", HTTP_POST, h_tls_recreate_do, NULL},
     #endif

    /* ----------------------- System API ------------------------ */
    #ifdef CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG
        /* Get logs from the device */
        {"/_/sys/logs", HTTP_GET,  h_sys_logs_txt,  NULL},
    #endif
    /* Getting information about the current system */
    {"/_/sys/info",     HTTP_GET,  h_sys_info_json, NULL},
    /* Reboot system (ESP) */
    {"/_/sys/reboot",   HTTP_POST, h_sys_reboot_do, NULL},

    /* ----------------------- OTA API ----------------------- */
    /* Getting information about the current firmware */
    {"/_/ota/info",     HTTP_GET,  h_ota_info_json,   NULL},
    /* Receiving new firmware */
    {"/_/ota/update",   HTTP_POST, h_ota_update_do,   NULL},
    /* Confirmation of a successful firmware update */
    {"/_/ota/confirm",  HTTP_POST, h_ota_confirm_do,  NULL},
    /* Rollback to previous firmware */
    {"/_/ota/rollback", HTTP_POST, h_ota_rollback_do, NULL},

    /* ---------------- Admin Password API ---------------- */
    /* Update Admin Password */
    {"/_/apass/update", HTTP_POST, h_apass_update_do, NULL},
};
const size_t http_server_default_handlers_count = sizeof(handlers) / sizeof(handlers[0]);

esp_err_t server_launch(void) {
    esp_err_t ret = ESP_OK;
    const eif_config_t * const cfg = eif_config_get();

    #ifdef CONFIG_EIF_ENABLE_TLS
        httpd_ssl_config_t conf_copy = cfg->server_config;
        EIF_TRY(httpd_ssl_start(&g_server_handle, &conf_copy));
    #else
        httpd_config_t conf_copy = cfg->server_config;
        EIF_TRY(httpd_start(&g_server_handle, &conf_copy));
    #endif

    EIF_LOG_I("Registering %d CORE URI handlers", http_server_default_handlers_count);
    EIF_TRY(register_uris_with_middleware(handlers,
                                          http_server_default_handlers_count, true));
    EIF_LOG_I("Registering %d CLIENT URI handlers", cfg->server_uri_handlers_count);
    EIF_TRY(register_uris_with_middleware(cfg->server_uri_handlers,
                                          cfg->server_uri_handlers_count, false));

    EIF_LOG_I("Launched successfully");

cleanup:
    if (ret == ESP_ERR_HTTPD_ALLOC_MEM) {
        EIF_LOG_E("Failed to allocate memory for %s, largest free block: %u",
                  TAG, heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
    } else if (ret != ESP_OK) {
        EIF_LOG_E("Launch failed, err: %s", esp_err_to_name(ret));
    }
    return ret;
}