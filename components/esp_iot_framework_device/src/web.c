/*
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Folder: ./components/esp_iot_framework_device/src
 * File: web.c
 * Library: esp_iot_framework_device
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

#include "device_macros.h"
#include "device_internal.h"
#include <esp_iot_framework_device.h>
#include <esp_iot_framework_core_ext.h>
#include <esp_iot_framework_core_macros.h>

#ifdef CONFIG_EIF_ENABLE_TLS
    #define TAG "HTTPS server"
#else
    #define TAG "HTTP server"
#endif


/* --- */

#define SERVER_ERR_JSON_PARSE      "JSON parsing failed"
#define SERVER_ERR_JSON_SER        "JSON serialization failed"
#define SERVER_ERR_JSON_MISSING    "Field `%s` is missing or invalid"
#define SERVER_ERR_NOT_FOUND_FIELD "Failed to get field '%s'"
#define SERVER_ERR_NVS_LOAD_PROF   "NVS load failed for profile \x23" "%d"
#define SERVER_ERR_NVS_SAVE_PROF   "NVS save failed in profile \x23" "%d"
#define SERVER_ERR_NVS_SAVE_APASS  "NVS save failed in Admin Password"
#define SERVER_ERR_ALLOCATE        "Failed to allocate %d bytes for '%s'"
#define SERVER_ERR_SPAWN_TASK      "Failed to spawn [%s]"
#define SERVER_ERR_INVALID_IDX     "Invalid '%s' index: %u (allowed range: %u-%u)"
#define SERVER_ERR_INVALID_LEN     "Invalid '%s' length: %u (allowed range: %u-%u)"

#define FIELD_WIFI_PROF_IDX     "profile_index"

#define HTTPD_202 "202 Accepted"
#define HTTPD_304 "304 Not Modified"
#define HTTPD_401 "401 Unauthorized"
#define HTTPD_409 "409 Conflict"

#if defined(CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ)
    #define EIF_CPU_FREQ_MHZ CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ
#elif defined(CONFIG_ESP32S2_DEFAULT_CPU_FREQ_MHZ)
    #define EIF_CPU_FREQ_MHZ CONFIG_ESP32S2_DEFAULT_CPU_FREQ_MHZ
#elif defined(CONFIG_ESP32S3_DEFAULT_CPU_FREQ_MHZ)
    #define EIF_CPU_FREQ_MHZ CONFIG_ESP32S3_DEFAULT_CPU_FREQ_MHZ
#elif defined(CONFIG_ESP32C3_DEFAULT_CPU_FREQ_MHZ)
    #define EIF_CPU_FREQ_MHZ CONFIG_ESP32C3_DEFAULT_CPU_FREQ_MHZ
#elif defined(CONFIG_ESP32C6_DEFAULT_CPU_FREQ_MHZ)
    #define EIF_CPU_FREQ_MHZ CONFIG_ESP32C6_DEFAULT_CPU_FREQ_MHZ
#else
    #define EIF_CPU_FREQ_MHZ 160
#endif

#define RESP_TYPE_JSON "application/json; charset=UTF-8"

#if defined(CONFIG_EIF_ENABLE_WEB_ADMIN_GUI) || defined(CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG)
    #define RESP_TYPE_TXT  "text/plain; charset=UTF-8"
#endif
#ifdef CONFIG_EIF_ENABLE_WEB_ADMIN_GUI
    #define RESP_TYPE_PNG  "image/png"
    #define RESP_TYPE_CSS  "text/css; charset=UTF-8"
    #define RESP_TYPE_HTML "text/html; charset=UTF-8"
    #define RESP_TYPE_JS   "application/javascript; charset=UTF-8"

    #define ESP_WARN_CACHE_HIT 1234
    #define ETAG_VALUE "\"" __DATE__ " " __TIME__ "\""
#endif

#ifdef CONFIG_EIF_ENABLE_BASIC_AUTH
    #define HTTP_BASIC_AUTH_REALM "Basic realm=\"web_basic_auth\""
#endif
#define SERVER_JSON_OUT_BUF_SIZE 1024
#define SERVER_JSON_IN_BUF_SIZE 256
#define SERVER_JSON_MAX_TOKENS 16U

#define HTTP_METHOD_MAX_LEN 8
#define HTTP_URI_MAX_LEN 16 * 1024
#define HTTP_LOGS_CHUNK_SIZE 512

#define HDR_CACHE_CONTROL_VALUE "public, max-age=" EIF_STR(CONFIG_EIF_WEB_CACHE_MAX_AGE)


/* Send */

#define BUFFER_TIMESTAMP 16U

static void httpd_resp_sendstatus(
    httpd_req_t * const req, const char * const status
) {
    #ifdef CONFIG_EIF_LOG_ENABLE_WEB_SEND_TIMESTAMP
        char body_buf[BUFFER_TIMESTAMP] = {0};
    #endif
    const char * p_body = "";

    #ifdef CONFIG_EIF_LOG_ENABLE_WEB_SEND_TIMESTAMP
        if (strcmp(status, HTTPD_204) != 0) {
            uint32_t ts = esp_log_timestamp();
            size_t idx = BUFFER_TIMESTAMP - 1U;
           
            body_buf[idx] = '\0';

            do {
                idx--;
                body_buf[idx] = "0123456789"[ts % 10U];
                ts /= 10U;
            } while (ts > 0U);

            p_body = &body_buf[idx];
        }
    #endif

    EIF_LOG_D("httpd_resp_sendstatus:status - %s", status);
    (void)httpd_resp_set_status(req, status);
    (void)httpd_resp_sendstr(req, p_body);
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

    EIF_IF_OK_CHECK_CONDITION(ret, (req->content_len == 0),
        ESP_ERR_INVALID_ARG, "Request body is empty, nothing to parse");

    EIF_IF_OK_CHECK_CONDITION(ret, 
        (req->content_len >= sizeof(ctx->buffer)),
        ESP_ERR_NO_MEM, "Static buffer too small");

    if (ret == ESP_OK) {
        received = httpd_req_recv(req, ctx->buffer, req->content_len);
        EIF_IF_OK_CHECK_CONDITION(ret, (received <= 0), 
            ESP_ERR_HTTPD_INVALID_REQ, "HTTP receive failed");
    }

    if (ret == ESP_OK) {
        ctx->buffer[received] = '\0';

        jsmn_parser parser;
        jsmn_init(&parser);
        
        int r = jsmn_parse(&parser, ctx->buffer,
            (size_t)received, ctx->tokens, SERVER_JSON_MAX_TOKENS);
        
        EIF_IF_OK_CHECK_CONDITION(ret, 
            ((r < 0) || (ctx->tokens[0].type != JSMN_OBJECT)),
            ESP_ERR_INVALID_STATE, SERVER_ERR_JSON_PARSE);

        if (ret == ESP_OK) {
            ctx->token_count = r;
        }
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

    EIF_IF_OK_CHECK_CONDITION(ret, 
        ((val_idx < 0) || (ctx->tokens[val_idx].type != JSMN_STRING)),
        ESP_ERR_INVALID_ARG, SERVER_ERR_JSON_MISSING, field);

    if (ret == ESP_OK) {
        const int diff = ctx->tokens[val_idx].end - ctx->tokens[val_idx].start;
        const size_t tok_len = (size_t)diff;

        EIF_IF_OK_CHECK_CONDITION(ret, 
            ((tok_len < min_limit) || (tok_len >= max_limit)), 
            ESP_ERR_INVALID_ARG, SERVER_ERR_INVALID_LEN, 
            field, tok_len, min_limit, max_limit - 1U);
            
        if (ret == ESP_OK) {
            for (size_t k = 0U; k < tok_len; k++) {
                size_t src_idx = (size_t)ctx->tokens[val_idx].start + k;
                out_value[k] = ctx->buffer[src_idx];
            }
            out_value[tok_len] = '\0';
        }
    }

    return ret;
}

static esp_err_t req_json_get_profile_index(
    const json_parser_ctx_t * const ctx, uint8_t * const index
) {
    esp_err_t ret = ESP_OK;
    int val_idx = -1;
    const uint8_t wifi_profiles_count = eif_wifi_get_profiles_count();
    const size_t field_len = strlen(FIELD_WIFI_PROF_IDX);

    for (int i = 1; i < (ctx->token_count - 1); i++) {
        if (ctx->tokens[i].type == JSMN_STRING) {
            const int diff = ctx->tokens[i].end - ctx->tokens[i].start;
            const size_t tok_len = (size_t)diff;
            
            if (tok_len == field_len) {
                size_t start_idx = (size_t)ctx->tokens[i].start;
                const char *str_ptr = &ctx->buffer[start_idx];
                
                if (strncmp(str_ptr, FIELD_WIFI_PROF_IDX, tok_len) == 0) {
                    val_idx = i + 1;
                    break;
                }
            }
        }
    }

    if ((val_idx < 0) || (ctx->tokens[val_idx].type != JSMN_PRIMITIVE)) {
        ret = ESP_ERR_INVALID_ARG;
        EIF_LOG_E(SERVER_ERR_JSON_MISSING, FIELD_WIFI_PROF_IDX);
    }

    if (ret == ESP_OK) {
        const int diff = ctx->tokens[val_idx].end - ctx->tokens[val_idx].start;
        const size_t tok_len = (size_t)diff;
        
        if ((tok_len == 0U) || (tok_len > 3U)) {
            ret = ESP_ERR_INVALID_ARG;
        } else {
            int valueint = 0;
            bool valid_number = true;

            for (size_t k = 0U; k < tok_len; k++) {
                size_t src_idx = (size_t)ctx->tokens[val_idx].start + k;
                char ch = ctx->buffer[src_idx];

                if ((ch >= '0') && (ch <= '9')) {
                    valueint = (valueint * 10) + ((int)ch - (int)'0');
                } else {
                    valid_number = false;
                    break;
                }
            }

            if (!valid_number || 
                (valueint > (int)wifi_profiles_count) || 
                (valueint > 255)) {
                ret = ESP_ERR_INVALID_ARG;
                EIF_LOG_E(
                    SERVER_ERR_INVALID_IDX, FIELD_WIFI_PROF_IDX, 
                    valueint, 0, wifi_profiles_count);
            } else {
                *index = (uint8_t)valueint;
            }
        }
    }

    return ret;
}




/* Files */

static esp_err_t set_cache(httpd_req_t * const req, bool is_need) {
    esp_err_t ret = ESP_OK;

    if (is_need) {
        #ifdef CONFIG_EIF_ENABLE_WEB_ADMIN_GUI
            char if_none_match[64] = {0};
            if (httpd_req_get_hdr_value_str(
                req, "If-None-Match", if_none_match, sizeof(if_none_match)
            ) == ESP_OK) {
                if (strcmp(if_none_match, ETAG_VALUE) == 0) {
                    httpd_resp_set_status(req, HTTPD_304);
                    httpd_resp_send(req, NULL, 0);
                    ret = ESP_WARN_CACHE_HIT;
                }
            }
            if (ret == ESP_OK) {
                httpd_resp_set_hdr(req, "ETag", ETAG_VALUE);
                httpd_resp_set_hdr(req, "Cache-Control", HDR_CACHE_CONTROL_VALUE);
            }
        #endif
    } else {
        httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate");
        httpd_resp_set_hdr(req, "Expires", "0");
        httpd_resp_set_hdr(req, "Connection", "close");
        httpd_resp_set_hdr(req, "Pragma", "no-cache");
    }

    /* Cleanup */
    return ret;
}

#ifdef CONFIG_EIF_ENABLE_WEB_ADMIN_GUI
    static esp_err_t httpd_resp_sendfile(
        httpd_req_t * const req, const eif_web_file_t * const file
    ) {
        esp_err_t ret = ESP_OK;

        /* @deviation [Rule 18.4] Pointer subtraction is justified here because the
         * asset length must be derived from two immutable bounds of a linker-defined
         * binary object. The start and end symbols are generated automatically from
         * an embedded HTTP resource, ensuring their existence and correct relative
         * ordering at link time. This operation is necessary to provide the exact
         * byte count required by the HTTP transmission API. The calculation is safe
         * because both pointers reference the same contiguous memory block, and no
         * out-of-bounds access is performed. */
        /* @deviation [Rule 10.8] The explicit cast to 'size_t' is required to convert
         * the pointer difference into the unsigned integer type expected by the web
         * server API. This conversion is safe in this context because the calculated
         * value represents the exact, valid size of the underlying linker-generated
         * resource and is used strictly as a buffer length parameter. */
        /* cppcheck-suppress misra-c2012-18.4 */
        /* cppcheck-suppress misra-c2012-10.8 */
        const size_t size = (size_t)(file->end - file->start);

        EIF_IF_OK_CHECK_ESP_ERR_T(ret, set_cache(req, file->need_cache),
            "Failed to set cache for '%s'", file->file_name);

        if (ret == ESP_OK) {
            httpd_resp_set_type(req, file->content_type);
            httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
            ret = httpd_resp_send(req, (const char *)file->start, size);
        } else if (ret == ESP_WARN_CACHE_HIT) {
            ret = ESP_OK;
        } else { ; }

        /* Cleanup */
        return ret;
    }

    /* @note Cppcheck 2.21 incorrectly reports a syntax error on this macro. The code
     * is valid and passes compilation with GCC/Clang. Excluded from linting to avoid
     * false positives. Run Cppcheck with -D__CPPCHECK__ flag.
     */
    #ifndef __CPPCHECK__
        #ifdef CONFIG_EIF_ENABLE_BASIC_AUTH
             EIF_DEFINE_HTTP_FILE(e401_html_gz, RESP_TYPE_HTML, false)
        #endif
        #ifdef CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG
            EIF_DEFINE_HTTP_FILE(logs_html_gz,    RESP_TYPE_HTML, true)
        #endif
        EIF_DEFINE_HTTP_FILE(e404_html_gz,    RESP_TYPE_HTML, false)
        EIF_DEFINE_HTTP_FILE(index_html_gz,   RESP_TYPE_HTML, true)
        EIF_DEFINE_HTTP_FILE(system_html_gz,  RESP_TYPE_HTML, true)
        EIF_DEFINE_HTTP_FILE(network_html_gz, RESP_TYPE_HTML, true)

        EIF_DEFINE_HTTP_FILE(style_css_gz,    RESP_TYPE_CSS,  true)

        EIF_DEFINE_HTTP_FILE(LICENSE_gz,      RESP_TYPE_TXT,  true)

        EIF_DEFINE_HTTP_FILE(json2_js_gz,     RESP_TYPE_JS,   true)
        EIF_DEFINE_HTTP_FILE(api_js_gz,       RESP_TYPE_JS,   true)

        #ifdef CONFIG_EIF_ENABLE_WEB_FAVICON
            EIF_DEFINE_HTTP_FILE(logo_png_gz,     RESP_TYPE_PNG,  true)
        #endif

        static esp_err_t httpd_err_404(httpd_req_t *req, httpd_err_code_t error) {
            EIF_LOG_W("HTTP 404 error from '%s'", req->uri);
            httpd_resp_set_status(req, HTTPD_404);
            return sendf_e404_html_gz(req);
        }
    #endif
#endif



/* WiFi */

static esp_err_t h_wifi_list_json(httpd_req_t * const req) {
    esp_err_t ret = ESP_OK;

    #if defined(CONFIG_EIF_ENABLE_TLS)
        #define EIF_TLS_VAL true
    #else
        #define EIF_TLS_VAL false
    #endif

    json_gen_str_t jgen = {0};
    wifi_ap_record_t info = {0};
    char ssid[EIF_WIFI_SSID_MAX_LEN] = {0};
    char pass[EIF_WIFI_PASS_MAX_LEN] = {0};
    const uint8_t wifi_profiles_count = eif_wifi_get_profiles_count();
    const uint8_t wifi_profiles_index = eif_wifi_get_current_profile_index();

    (void)set_cache(req, false);

    json_gen_str_start(&jgen,
        g_server_json_buffer, SERVER_JSON_OUT_BUF_SIZE, NULL, NULL);
    /* { */
    (void)json_gen_start_object(&jgen);
    /*   "current_profile_index": 2, */
    (void)json_gen_obj_set_int(&jgen, "current_profile_index", wifi_profiles_index);
    /*   "used_tls": true, */
    (void)json_gen_obj_set_int(&jgen, "used_tls", EIF_TLS_VAL);
    /*   "rssi_now_profile": -45, */
    if (esp_wifi_sta_get_ap_info(&info) == ESP_OK) {
        (void)json_gen_obj_set_int(&jgen, "rssi_now_profile", info.rssi);
    }
    /*   "profiles": [ */
    (void)json_gen_push_array(&jgen, "profiles");

    for (uint16_t idx = 0U; idx <= (uint16_t)wifi_profiles_count; idx++) {
        EIF_IF_OK_CHECK_ESP_ERR_T(ret,
            eif_nvs_wifi_profile_load(idx, ssid, pass),
            SERVER_ERR_NVS_LOAD_PROF, idx);

        if (ret == ESP_OK) {
            /* { */
            (void)json_gen_start_object(&jgen);
            /*   "ssid": ESP32_SETUP", */
            (void)json_gen_obj_set_string(&jgen, "ssid", ssid);
            if (idx == 0U) {
            /*   "password": 12345678" */
                (void)json_gen_obj_set_string(&jgen, "password", pass);
            }
            /* }, */
            (void)json_gen_end_object(&jgen);
        } else {
            break;
        }
    }

    /*   ] */
    (void)json_gen_pop_array(&jgen);
    /* } */
    (void)json_gen_end_object(&jgen);

    int json_len = json_gen_str_end(&jgen);
    EIF_IF_OK_CHECK_CONDITION(ret,
        ((json_len <= 0) || (json_len > SERVER_JSON_OUT_BUF_SIZE)),
        ESP_ERR_NO_MEM, SERVER_ERR_JSON_SER);

    if (ret == ESP_OK) {
        httpd_resp_set_type(req, RESP_TYPE_JSON);
        httpd_resp_sendstr(req, g_server_json_buffer);
    } else {
        httpd_resp_sendstatus(req, HTTPD_500);
    }

    /* Cleanup */
    return ret;
}

static esp_err_t h_wifi_update_do(httpd_req_t * const req) {
    esp_err_t ret = ESP_OK;

    uint8_t index = 0;
    json_parser_ctx_t parser_ctx = {0};
    char ssid[EIF_WIFI_SSID_MAX_LEN] = {0};
    char pass[EIF_WIFI_PASS_MAX_LEN] = {0};

    (void)set_cache(req, false);

    EIF_IF_OK_CHECK_ESP_ERR_T(ret,
        req_http_parse_json(req, &parser_ctx), SERVER_ERR_JSON_PARSE);

    EIF_IF_OK_CHECK_ESP_ERR_T(ret,
        req_json_get_profile_index(&parser_ctx, &index),
        SERVER_ERR_NOT_FOUND_FIELD, FIELD_WIFI_PROF_IDX);

    EIF_IF_OK_CHECK_ESP_ERR_T(ret, req_json_get_field(&parser_ctx,
        ssid, "ssid", EIF_WIFI_SSID_MIN_LEN, EIF_WIFI_SSID_MAX_LEN
    ), SERVER_ERR_NOT_FOUND_FIELD, "ssid");

    EIF_IF_OK_CHECK_ESP_ERR_T(ret, req_json_get_field(&parser_ctx,
        pass, "password", EIF_WIFI_PASS_MIN_LEN, EIF_WIFI_PASS_MAX_LEN
    ), SERVER_ERR_NOT_FOUND_FIELD, "password");

    if (ret == ESP_ERR_NO_MEM) {
        httpd_resp_sendstatus(req, HTTPD_500);
    } else if (ret != ESP_OK) {
        httpd_resp_sendstatus(req, HTTPD_400);
    } else { /* ret == ESP_OK */
        EIF_IF_OK_CHECK_ESP_ERR_T(ret,
            eif_nvs_wifi_profile_save(index, ssid, pass),
            SERVER_ERR_NVS_SAVE_PROF, index);

        if (ret == ESP_OK) {
            httpd_resp_sendstatus(req, HTTPD_204);
        } else {
            httpd_resp_sendstatus(req, HTTPD_500);
        }
    }

    /* Cleanup */
    return ret;
}

static esp_err_t h_wifi_clear_do(httpd_req_t * const req) {
    esp_err_t ret = ESP_OK;

    uint8_t index = 0;
    json_parser_ctx_t parser_ctx = {0};

    (void)set_cache(req, false);

    EIF_IF_OK_CHECK_ESP_ERR_T(ret,
        req_http_parse_json(req, &parser_ctx), SERVER_ERR_JSON_PARSE);

    EIF_IF_OK_CHECK_ESP_ERR_T(ret,
        req_json_get_profile_index(&parser_ctx, &index),
        SERVER_ERR_NOT_FOUND_FIELD, FIELD_WIFI_PROF_IDX);

    if (ret != ESP_OK) {
        httpd_resp_sendstatus(req, HTTPD_400);
    } else {
        EIF_IF_OK_CHECK_ESP_ERR_T(ret,
            eif_nvs_wifi_profile_save(index, "", ""),
            SERVER_ERR_NVS_SAVE_PROF, index);

        if (ret == ESP_OK) {
            httpd_resp_sendstatus(req, HTTPD_204);
        } else {
            httpd_resp_sendstatus(req, HTTPD_500);
        }
    }

    /* Cleanup */
    return ret;
}

static esp_err_t h_wifi_check_do(httpd_req_t * const req) {
    esp_err_t ret = ESP_OK;

    uint8_t index = 0;
    json_parser_ctx_t parser_ctx = {0};

    (void)set_cache(req, false);

    EIF_IF_OK_CHECK_ESP_ERR_T(ret,
        req_http_parse_json(req, &parser_ctx), SERVER_ERR_JSON_PARSE);

    EIF_IF_OK_CHECK_ESP_ERR_T(ret,
        req_json_get_profile_index(&parser_ctx, &index),
        SERVER_ERR_NOT_FOUND_FIELD, FIELD_WIFI_PROF_IDX);

    if (ret != ESP_OK) {
        httpd_resp_sendstatus(req, HTTPD_400);
    } else {
        EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_task_wifi_test_launch(index),
            SERVER_ERR_SPAWN_TASK, "wifi_test");
       
        if (ret != ESP_OK) {
            httpd_resp_sendstatus(req, HTTPD_500);
        } else {
            httpd_resp_sendstatus(req, HTTPD_202);
            EIF_LOG_I("WiFi test task launched for profile #%u", index);
        }
    }

    /* Cleanup */
    return ret;
}

static esp_err_t h_wifi_result_json(httpd_req_t * const req) {
    esp_err_t ret = ESP_OK;

    uint8_t index = 0;
    json_gen_str_t jgen = {0};
    json_parser_ctx_t parser_ctx = {0};
    eif_wifi_test_result test_res = {0};
    
    (void)set_cache(req, false);

    EIF_IF_OK_CHECK_ESP_ERR_T(ret,
        req_http_parse_json(req, &parser_ctx), SERVER_ERR_JSON_PARSE);

    EIF_IF_OK_CHECK_ESP_ERR_T(ret,
        req_json_get_profile_index(&parser_ctx, &index),
        SERVER_ERR_NOT_FOUND_FIELD, FIELD_WIFI_PROF_IDX);

    if (ret != ESP_OK) {
        httpd_resp_sendstatus(req, HTTPD_400);
    } else {
        json_gen_str_start(&jgen,
            g_server_json_buffer, SERVER_JSON_OUT_BUF_SIZE, NULL, NULL);

        EIF_IF_OK_CHECK_ESP_ERR_T(ret,
            eif_wifi_get_test_result(index, &test_res),
            "Couldn't upload Wi-Fi profile test results");

        if (ret == ESP_OK) {
            /* { */
            (void)json_gen_start_object(&jgen);
            /*   "result": true, */
            (void)json_gen_obj_set_bool(&jgen, "result", test_res.connected);
            /*   "rssi": -45 */
            (void)json_gen_obj_set_int(&jgen, "rssi", test_res.rssi);
            /* } */
            (void)json_gen_end_object(&jgen);

            int json_len = json_gen_str_end(&jgen);
            EIF_IF_OK_CHECK_CONDITION(ret,
                ((json_len <= 0) || (json_len > SERVER_JSON_OUT_BUF_SIZE)),
                ESP_ERR_NO_MEM, SERVER_ERR_JSON_SER);
        }

        if (ret == ESP_OK) {
            httpd_resp_set_type(req, RESP_TYPE_JSON);
            httpd_resp_sendstr(req, g_server_json_buffer);
        } else {
            httpd_resp_sendstatus(req, HTTPD_500);
        }
    }

    /* Cleanup */
    return ret;
}



/* TLS */

#ifdef CONFIG_EIF_ENABLE_TLS
    static esp_err_t h_tls_recreate_do(httpd_req_t * const req) {
        esp_err_t ret = ESP_OK;

        (void)set_cache(req, false);

        EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_task_tls_recreate_launch(),
            SERVER_ERR_SPAWN_TASK, "tls_recreate");

        if (ret == ESP_OK) {
            httpd_resp_sendstatus(req, HTTPD_202);
        } else {
            httpd_resp_sendstatus(req, HTTPD_500);
        }

        /* Cleanup */
        return ret;
    }
#endif



/* System */

static esp_err_t h_sys_info_json(httpd_req_t * const req) {
    esp_err_t ret = ESP_OK;

    uint8_t mac[6] = {0};
    char mac_str[18] = {0};
    uint32_t flash_size = 0;
    json_gen_str_t jgen = {0};
    esp_chip_info_t chip = {0};
    const char hex_chars[] = "0123456789ABCDEF";
    int largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);

    (void)set_cache(req, false);
    
    esp_chip_info(&chip);
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    esp_flash_get_size(NULL, &flash_size);

    {
        size_t pos = 0U;

        for (size_t i = 0U; i < 6U; i++) {
            size_t hi = ((size_t)mac[i] >> 4U) & 0x0FU;
            size_t lo = (size_t)mac[i] & 0x0FU;

            mac_str[pos] = hex_chars[hi];
            pos++;
            mac_str[pos] = hex_chars[lo];
            pos++;

            if (i < 5U) {
                mac_str[pos] = ':';
                pos++;
            }
        }
        
        mac_str[pos] = '\0';
    }

    json_gen_str_start(&jgen,
        g_server_json_buffer, SERVER_JSON_OUT_BUF_SIZE, NULL, NULL);

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
    (void)json_gen_obj_set_int(&jgen, "cpu_freq",  EIF_CPU_FREQ_MHZ);
    /*   "reset_reason": 1, */
    (void)json_gen_obj_set_int(&jgen, "reset_reason", esp_rom_get_reset_reason(0));
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

    int json_len = json_gen_str_end(&jgen);
    EIF_IF_OK_CHECK_CONDITION(ret, json_len <= 0,
        ESP_ERR_NO_MEM, SERVER_ERR_JSON_SER);

    if (ret == ESP_OK) {
        httpd_resp_set_type(req, RESP_TYPE_JSON);
        httpd_resp_sendstr(req, g_server_json_buffer);
    } else {
        httpd_resp_sendstatus(req, HTTPD_500);
    }

    /* Cleanup */
    return ret;
}

static esp_err_t h_sys_reboot_do(httpd_req_t * const req) {
    esp_err_t ret = ESP_OK;

    (void)set_cache(req, false);

    EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_task_reboot_launch(),
        SERVER_ERR_SPAWN_TASK, "reboot_system");

    if (ret == ESP_OK) {
        httpd_resp_sendstatus(req, HTTPD_202);
    } else {
        httpd_resp_sendstatus(req, HTTPD_500);
    }

    /* Cleanup */
    return ret;
}

#ifdef CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG
    static esp_err_t h_sys_logs_txt(httpd_req_t *req) {
        esp_err_t ret = ESP_OK;
        char tx_buffer[HTTP_LOGS_CHUNK_SIZE] = {0};
        
        (void)httpd_resp_set_type(req, RESP_TYPE_TXT);

        size_t bytes_read = eif_core_log_pop_chunk(tx_buffer, HTTP_LOGS_CHUNK_SIZE);

        while (bytes_read > 0U) {
            EIF_IF_OK_CHECK_ESP_ERR_T(ret,
                httpd_resp_send_chunk(req, tx_buffer, bytes_read),
                "Failed to send log chunk of %u bytes (max: %u)",
                bytes_read, HTTP_LOGS_CHUNK_SIZE);
                
            if (ret != ESP_OK) {
                break;
            }

            bytes_read = eif_core_log_pop_chunk(tx_buffer, HTTP_LOGS_CHUNK_SIZE);
        }

        if (ret == ESP_OK) {
            (void)httpd_resp_send_chunk(req, NULL, 0U);
        }

        return ret;
    }
#endif



/* OTA */

static esp_err_t h_ota_info_json(httpd_req_t * const req) {
    esp_err_t ret = ESP_OK;

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

    (void)set_cache(req, false);

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

    json_gen_str_start(&jgen,
        g_server_json_buffer, SERVER_JSON_OUT_BUF_SIZE, NULL, NULL);

    /* { */
    (void)json_gen_start_object(&jgen);
    /*   "project": "my_iot_project", */
    (void)json_gen_obj_set_string(&jgen, "project", app->project_name);
    /*   "version": "1.0.0", */
    (void)json_gen_obj_set_string(&jgen, "version", app->version);
    /*   "build_id": "a1b2c3d4e5f6...", */
    (void)json_gen_obj_set_string(&jgen, "build_id", sha_str);
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

    int json_len = json_gen_str_end(&jgen);
    EIF_IF_OK_CHECK_CONDITION(ret, json_len <= 0,
        ESP_ERR_NO_MEM, SERVER_ERR_JSON_SER);
    
    if (ret == ESP_OK) {
        httpd_resp_set_type(req, RESP_TYPE_JSON);
        httpd_resp_sendstr(req, g_server_json_buffer);
    } else {
        httpd_resp_sendstatus(req, HTTPD_500);
    }

    /* Cleanup */
    return ret;
}

static esp_err_t h_ota_update_do(httpd_req_t * const req) {
    static bool is_ota_busy = false;

    esp_err_t ret = ESP_OK;

    char * buf = NULL;
    esp_ota_handle_t update_handle = 0;
    size_t remaining = req->content_len;
    const esp_partition_t * update_partition = NULL;

    (void)set_cache(req, false);

    if (is_ota_busy) {
        EIF_LOG_W("OTA Update already in progress. Rejecting request.");
        ret = ESP_ERR_INVALID_STATE;
    } else {
        is_ota_busy = true;
    }

    if (ret == ESP_OK) {
        update_partition = esp_ota_get_next_update_partition(NULL);
        EIF_IF_OK_CHECK_CONDITION(ret, update_partition == NULL,
            ESP_FAIL, "No OTA partition");
    }

    if (ret == ESP_OK) {
        buf = (char *)pvPortMalloc(CONFIG_EIF_WEB_SIZE_OTA_BUFFER);
        EIF_IF_OK_CHECK_CONDITION(ret, buf == NULL, ESP_ERR_NO_MEM,
            SERVER_ERR_ALLOCATE, CONFIG_EIF_WEB_SIZE_OTA_BUFFER, "ota_buffer");
    }
       
    while ((remaining > 0U) && (ret == ESP_OK)) {
        const bool flag = remaining < (size_t)CONFIG_EIF_WEB_SIZE_OTA_BUFFER;
        const int chunk_size = flag ? (int)remaining : CONFIG_EIF_WEB_SIZE_OTA_BUFFER;
        const int received = httpd_req_recv(req, buf, chunk_size);

        if (received <= 0) {
            if (received != HTTPD_SOCK_ERR_TIMEOUT) {
                ret = ESP_FAIL;
            }
        } else {
            if ((update_handle == 0) && (ret == ESP_OK)) {
                const uint8_t magic_byte = (uint8_t)buf[0];
                EIF_IF_OK_CHECK_CONDITION(ret, magic_byte != 0xE9U,
                    ESP_ERR_INVALID_ARG, "Invalid magic byte");

                EIF_IF_OK_CHECK_ESP_ERR_T(ret, esp_ota_begin(
                    update_partition, OTA_SIZE_UNKNOWN, &update_handle
                ), "Begin failed");
            }

            EIF_IF_OK_CHECK_ESP_ERR_T(ret, esp_ota_write(
                update_handle, (const void *)buf, received
            ), "Write failed");
           
            if (ret == ESP_OK) { 
                remaining -= (size_t)received;
            }
        }
    }

    if (ret == ESP_OK) {
        EIF_IF_OK_CHECK_ESP_ERR_T(ret, esp_ota_end(update_handle),
            "End failed");
    }

    EIF_IF_OK_CHECK_ESP_ERR_T(ret,
        esp_ota_set_boot_partition(update_partition), "Boot err");

    if (buf != NULL) {
        vPortFree(buf);
    }

    if (ret == ESP_OK) {
        update_handle = 0;
        httpd_resp_sendstatus(req, HTTPD_204);
    } else if (ret == ESP_ERR_INVALID_STATE) {
        httpd_resp_sendstatus(req, HTTPD_409);
    } else if (ret == ESP_ERR_INVALID_ARG) {
        httpd_resp_sendstatus(req, HTTPD_400);
    } else {
        httpd_resp_sendstatus(req, HTTPD_500);
    }

    EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_task_reboot_launch(),
        SERVER_ERR_SPAWN_TASK, "reboot_system");

    /* Cleanup */
    if ((ret != ESP_OK) && (ret != ESP_ERR_INVALID_STATE)) {
        is_ota_busy = false;
    }
    if ((ret != ESP_OK) && (update_handle != 0)) {
        esp_ota_abort(update_handle);
    }

    return ret;
}

static esp_err_t h_ota_action_do(httpd_req_t * const req) {
    esp_err_t ret = ESP_OK;

    (void)set_cache(req, false);

    if (strncmp(req->uri, "/_/ota/confirm.do", 17) == 0) {
        ret = esp_ota_mark_app_valid_cancel_rollback();

        if (ret == ESP_OK) {
            httpd_resp_sendstatus(req, HTTPD_202);
        } else {
            httpd_resp_sendstatus(req, HTTPD_500);
        }
    } else if (strncmp(req->uri, "/_/ota/rollback.do", 18) == 0) {
        EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_task_rollback_and_reboot_launch(),
            SERVER_ERR_SPAWN_TASK, "rollback_and_reboot");

        if (ret == ESP_OK) {
            httpd_resp_sendstatus(req, HTTPD_202);
        } else {
            httpd_resp_sendstatus(req, HTTPD_500);
        }
    } else {
        httpd_resp_sendstatus(req, HTTPD_400);
        ret = ESP_FAIL;
    }

    /* Cleanup */
    return ret;
}



/* Basic Auth */

#ifdef CONFIG_EIF_ENABLE_BASIC_AUTH
    static esp_err_t h_apass_update_do(httpd_req_t * const req) {
        esp_err_t ret = ESP_OK;

        json_parser_ctx_t parser_ctx = {0};
        char pass[EIF_BASIC_AUTH_PASS_MAX_LEN] = {0};

        EIF_IF_OK_CHECK_ESP_ERR_T(ret,
            req_http_parse_json(req, &parser_ctx), SERVER_ERR_JSON_PARSE);

        EIF_IF_OK_CHECK_ESP_ERR_T(ret, req_json_get_field(
            &parser_ctx, pass, "password",
            EIF_BASIC_AUTH_PASS_MIN_LEN, EIF_BASIC_AUTH_PASS_MAX_LEN
        ), SERVER_ERR_NOT_FOUND_FIELD, "password");

        if (ret != ESP_OK) {
            httpd_resp_sendstatus(req, HTTPD_400);
        } else {
            EIF_IF_OK_CHECK_ESP_ERR_T(ret,
                eif_nvs_basic_auth_line_save((uint8_t *)pass),
                SERVER_ERR_NVS_SAVE_APASS);

            if (ret == ESP_OK) {
                httpd_resp_sendstatus(req, HTTPD_204);
            } else {
                httpd_resp_sendstatus(req, HTTPD_500);
            }
        }

        /* Cleanup */
        return ret;
    }
#endif



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

#ifdef CONFIG_EIF_ENABLE_BASIC_AUTH
    static esp_err_t req_check_auth_admin(httpd_req_t *req) {
        esp_err_t ret = ESP_OK;

        /* @note +8 bytes to the buffer avoids password truncation.
         * Example (imagine that 'AUTH_LINE_MAX_LEN' = 18):
         *   Header: Basic YWRtaW46MTIzc2Q=  (22 bytes)
         *   NVS:    Basic YWRtaW46MTIz      (18 bytes)
         * In this case, without +8 bytes, the data from the header
         * will be truncated to the length of the NVS and the comparison
         * will be successful. */
        char buf[EIF_BASIC_AUTH_LINE_MAX_LEN + 8U] = {0};
        char current_pw[EIF_BASIC_AUTH_LINE_MAX_LEN] = {0};

        EIF_IF_OK_CHECK_ESP_ERR_T(ret, httpd_req_get_hdr_value_str(
            req, "Authorization", buf, sizeof(buf)
        ), "Get 'Authorization' header error");

        if ((ret != ESP_OK) && (ret != ESP_ERR_NOT_FOUND)) {
            (void)set_cache(req, false);
            httpd_resp_sendstatus(req, HTTPD_400);
        }

        if (ret == ESP_OK) {
            EIF_IF_OK_CHECK_ESP_ERR_T(ret,
                eif_nvs_basic_auth_line_load(current_pw),
                "Failed to load basic auth data");

            if ((ret != ESP_OK) && (ret != ESP_ERR_NOT_FOUND)) {
                (void)set_cache(req, false);
                httpd_resp_sendstatus(req, HTTPD_500);
            }
        }

        if ((ret == ESP_ERR_NOT_FOUND) || (strcmp(buf, current_pw) != 0)) {
            (void)set_cache(req, false);
            EIF_LOG_W("UNAUTHORIZED");

            httpd_resp_set_hdr(req, "WWW-Authenticate", HTTP_BASIC_AUTH_REALM);

            #ifdef CONFIG_EIF_ENABLE_WEB_ADMIN_GUI
                httpd_resp_set_status(req, HTTPD_401);
                (void)sendf_e401_html_gz(req);
            #else
                (void)httpd_resp_sendstatus(req, HTTPD_401);
            #endif
        }

        return ret;
    }
#endif

static esp_err_t middleware_universal(httpd_req_t * const req) {
    esp_err_t ret = ESP_OK;
    uint32_t start_time = esp_log_timestamp();

    EIF_LOG_I("%s %s HTTP/1.1", method_to_str(req->method), req->uri);

    const httpd_uri_t* orig = (const httpd_uri_t*)req->user_ctx;
    esp_err_t (*real_handler)(httpd_req_t *) = (esp_err_t (*)(httpd_req_t *))orig->handler;

    #ifdef CONFIG_EIF_ENABLE_BASIC_AUTH
        if (strncmp(req->uri, "/_/", 3) == 0) {
            ret = req_check_auth_admin(req);
            if (ret != ESP_OK) {
                EIF_LOG_W("Checker Basic Auth error: %s", esp_err_to_name(ret));
            }
        }
    #endif

    if ((ret == ESP_OK) && (real_handler != NULL)) {
        req->user_ctx = orig->user_ctx;
        ret = real_handler(req);
        EIF_LOG_I("Result(%u): %s", (size_t)start_time, esp_err_to_name(ret));
    }

    (void)start_time;
    
    /* Cleanup */
    return ret;
}

static esp_err_t register_uris_with_middleware(
    const httpd_uri_t * const routes, size_t route_count,
    esp_err_t (* mw_func)(httpd_req_t *)
) {
    esp_err_t ret = ESP_OK;

    if (route_count == 0U) {
        EIF_LOG_W("No handlers to register");
    } else {
        EIF_IF_OK_CHECK_NOT_NULL(ret, routes, ESP_ERR_INVALID_ARG);

        size_t max_method_len = 0;
        size_t max_uri_len = 0;

        if (ret == ESP_OK) {
            for (size_t i = 0; i < route_count; i++) {
                const char * const method_str = method_to_str(routes[i].method);
                size_t method_len = eif_strnlen(method_str, HTTP_METHOD_MAX_LEN);
                if (method_len > max_method_len) {
                    max_method_len = method_len;
                }
                size_t uri_len = eif_strnlen(routes[i].uri, HTTP_URI_MAX_LEN);
                if (uri_len > max_uri_len) {
                    max_uri_len = uri_len;
                }
            }
        }

        for (size_t i = 0; i < route_count; i++) {
            if (ret == ESP_OK) {
                httpd_uri_t route_to_reg = routes[i];
           
                route_to_reg.user_ctx = (void *)&routes[i];
                route_to_reg.handler = mw_func;

                EIF_SHOW_ESP_ERR_T(ret,
                    httpd_register_uri_handler(g_server_handle, &route_to_reg),
                    "Failed to register URI: %s", routes[i].uri);
               
                if (ret == ESP_OK) {
                    EIF_LOG_I("Registered: %-*s %-*s HTTP/1.1",
                        (int)max_method_len, method_to_str(routes[i].method),
                        (int)max_uri_len, routes[i].uri);
                }
            }
        }
    }

    /* Cleanup */
    return ret;
}

esp_err_t eif_server_stop(void) {
    if (g_server_handle != NULL) {
        vTaskDelay(pdMS_TO_TICKS(100));
        #ifdef CONFIG_EIF_ENABLE_TLS
            httpd_ssl_stop(g_server_handle);
        #else
            httpd_stop(g_server_handle);
        #endif
        g_server_handle = NULL;
        vTaskDelay(pdMS_TO_TICKS(100));
        EIF_LOG_W("Server is stopped");
    } else {
        EIF_LOG_W("Server is not running or already stopped");
    }

    /* Cleanup */
    return ESP_OK;
}

esp_err_t eif_server_launch(void) {
    static const httpd_uri_t handlers[] = {
        #ifdef CONFIG_EIF_ENABLE_WEB_ADMIN_GUI
            /* ------------------------- Files ------------------------ */
            #ifdef CONFIG_EIF_ENABLE_WEB_FAVICON
                {"/favicon.ico",       HTTP_GET, sendf_logo_png_gz,    NULL},
            #endif 
            #ifdef CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG
                {"/_/files/logs.html", HTTP_GET, sendf_logs_html_gz,   NULL},
            #endif
            {"/_/files/api.js",       HTTP_GET, sendf_api_js_gz,       NULL},
            {"/_/files/json2.js",     HTTP_GET, sendf_json2_js_gz,     NULL},

            {"/_/files/style.css",    HTTP_GET, sendf_style_css_gz,    NULL},
            {"/_/files/license.txt",  HTTP_GET, sendf_LICENSE_gz,      NULL},
            
            {"/_/files/index.html",   HTTP_GET, sendf_index_html_gz,   NULL},
            {"/_/files/system.html",  HTTP_GET, sendf_system_html_gz,  NULL},
            {"/_/files/network.html", HTTP_GET, sendf_network_html_gz, NULL},
        #endif

        /* ----------------------- WiFi API ---------------------- */
        /* Get parameters of all WiFi profiles */
        {"/_/wifi/list.json",   HTTP_GET,  h_wifi_list_json,   NULL},
        /* Update WiFi profile under index X */
        {"/_/wifi/update.do",   HTTP_POST, h_wifi_update_do,   NULL},
        /* Clear WiFi profile under index X */
        {"/_/wifi/clear.do",    HTTP_POST, h_wifi_clear_do,    NULL},
        /* Check WiFi availability using parameters under index X */
        {"/_/wifi/check.do",    HTTP_POST, h_wifi_check_do,    NULL},
        /* Getting the result from 'POST /_/wifi/check.do' */
        {"/_/wifi/result.json", HTTP_POST, h_wifi_result_json, NULL},

        #ifdef CONFIG_EIF_ENABLE_TLS
            /* ----------------------- TLS API ---------------- ---- */
            /* Generate new TLS keys and certificate */
            {"/_/tls/recreate.do", HTTP_POST, h_tls_recreate_do, NULL},
        #endif

        /* ----------------------- System API ------------------------ */
        /* Getting information about the current system */
        {"/_/sys/info.json", HTTP_GET,  h_sys_info_json, NULL},
        /* Reboot system (ESP) */
        {"/_/sys/reboot.do", HTTP_POST, h_sys_reboot_do, NULL},
        #ifdef CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG
            {"/_/sys/logs.txt", HTTP_GET, h_sys_logs_txt, NULL},
        #endif

        /* ----------------------- OTA API ----------------------- */
        /* Getting information about the current firmware */
        {"/_/ota/info.json",   HTTP_GET,  h_ota_info_json, NULL},
        /* Receiving new firmware */
        {"/_/ota/update.do",   HTTP_POST, h_ota_update_do, NULL},
        /* Confirmation of a successful firmware update */
        {"/_/ota/confirm.do",  HTTP_POST, h_ota_action_do, NULL},
        /* Rollback to previous firmware */
        {"/_/ota/rollback.do", HTTP_POST, h_ota_action_do, NULL},

        #ifdef CONFIG_EIF_ENABLE_BASIC_AUTH
            /* ---------------- Admin Password API ---------------- */
            /* Update Admin Password (for Web GUI) */
            {"/_/apass/update.do", HTTP_POST, h_apass_update_do, NULL},
        #endif
    };

    esp_err_t ret = ESP_OK;

    const eif_device_t * const cfg = eif_device_get();
    int handlers_count = sizeof(handlers) / sizeof(handlers[0]);

    EIF_LOG_I("Starting server launch");

    if (g_server_handle != NULL) {
        EIF_LOG_W("There is an old server: %p, must be deleted", g_server_handle);
        (void)eif_server_stop();
        vTaskDelay(pdMS_TO_TICKS(100));
        EIF_LOG_D("Old server cleared: %p", g_server_handle);
    }

    #ifdef CONFIG_EIF_ENABLE_TLS
        EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_set_tls_creds_from_nvs(),
            "Failed to set TLS credentials");
        httpd_ssl_config_t conf_copy = cfg->server_config;
        EIF_IF_OK_CHECK_ESP_ERR_T(ret,
            httpd_ssl_start(&g_server_handle, &conf_copy), "httpd_ssl_start failed");
    #else
        httpd_config_t conf_copy = cfg->server_config;
        EIF_IF_OK_CHECK_ESP_ERR_T(ret,
            httpd_start(&g_server_handle, &conf_copy), "httpd_start failed");
    #endif

    if (ret == ESP_OK) {
        EIF_LOG_I("Server handle created: %p", g_server_handle);
        EIF_LOG_I("Registering %d CORE URI handlers", handlers_count);
    }

    EIF_IF_OK_CHECK_ESP_ERR_T(ret, register_uris_with_middleware(
        handlers, handlers_count, middleware_universal
    ), "Failed to register CORE URIs");

    #ifdef CONFIG_EIF_ENABLE_WEB_ADMIN_GUI
        EIF_IF_OK_CHECK_ESP_ERR_T(ret, httpd_register_err_handler(
            g_server_handle, HTTPD_404_NOT_FOUND, httpd_err_404
        ), "Failed to register handler '404 error'");
    #endif

    if (ret == ESP_OK) {
        EIF_LOG_I("Registering %d CLIENT URI handlers", cfg->uri_handlers_count);
    }
    EIF_IF_OK_CHECK_ESP_ERR_T(ret, register_uris_with_middleware(
        cfg->uri_handlers, cfg->uri_handlers_count, middleware_universal
    ), "Failed to register CLIENT URIs");

    if (ret == ESP_OK) {
        EIF_LOG_I("%s launched successfully", TAG);
    }

    /* Cleanup */
    return ret;
}