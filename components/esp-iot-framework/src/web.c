/*
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp_iot_framework
 * Folder: src
 * File: web.c
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

#include "cJSON.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "esp_flash.h"
#include "sdkconfig.h"
#include "esp_ota_ops.h"
#include "esp_chip_info.h"
#include "esp_app_format.h"
#ifdef CONFIG_EIF_ENABLE_TLS
    #include "esp_https_server.h"
#else
    #include "esp_http_server.h"
#endif



#include "macros.h"
#include "core_internal.h"
#include "esp_iot_framework.h"

#ifdef CONFIG_EIF_ENABLE_TLS
    #define TAG "HTTPS server"
#else
    #define TAG "HTTP server"
#endif

/* --- */

#define _ERR_JSON_NO_MEM     "Failed to create JSON root"
#define _ERR_JSON_PARSE      "JSON parsing failed"
#define _ERR_JSON_SER        "JSON serialization failed"
#define _ERR_JSON_MISSING    "Field `%s` is missing or invalid"
#define _ERR_NOT_FOUND_FIELD "Failed to get field '%s'"
#define _ERR_NVS_LOAD_PROF   "NVS load failed for profile #%d"
#define _ERR_NVS_SAVE_PROF   "NVS save failed in profile #%d"
#define _ERR_NVS_SAVE_APASS  "NVS save failed in Admin Password"
#define _ERR_ALLOCATE        "Failed to allocate %d bytes for '%s'"
#define _ERR_BASE64_ENCODE   "Base64 encode failed! Err: %d, Len: %u"
#define _ERR_SPAWN_TASK      "Failed to spawn [%s]. Free heap: %zu bytes"
#define _ERR_INVALID_IDX     "Invalid '%s' index: %u (allowed range: %u-%u)"
#define _ERR_INVALID_LEN     "Invalid '%s' length: %u (allowed range: %u-%u)"

#define _F_WIFI_PROF_IDX     "profile_index"
#define _F_WIFI_PROF_IDX_CUR "current_profile_index"
#define _F_WIFI_SSID         "ssid"
#define _F_WIFI_PASS         "password"
#define _F_WIFI_RSSI         "rssi"
#define _F_WIFI_RSSI_PROF    "rssi_now_profile"
#define _F_WIFI_PROFS        "profiles"
#define _F_WIFI_RESULT       "result"

#define _F_APASS_PASS        "password"

#define _F_OTA_PROJECT       "project"
#define _F_OTA_VER           "version"
#define _F_OTA_BUILD_ID      "build_id"
#define _F_OTA_BUILD_DATE    "build_date"
#define _F_OTA_BUILD_TIME    "build_time"
#define _F_OTA_IDF_VER       "idf_version"
#define _F_OTA_GCC_VER       "compiler"
#define _F_OTA_TARGET        "target"
#define _F_OTA_PARTITION     "partition"
#define _F_OTA_STATUS        "ota_status"

#define _F_SYS_HEAP_FREE     "heap_free"
#define _F_SYS_HEAP_MIN      "heap_min"
#define _F_SYS_LARG_BLOCK    "largest_block"
#define _F_SYS_UPTIME        "uptime"
#define _F_SYS_CPU_FREQ      "cpu_freq"
#define _F_SYS_CORES         "cores"
#define _F_SYS_CHIP_MODEL    "chip_model"
#define _F_SYS_CHIP_REV      "chip_rev"
#define _F_SYS_FLASH_SIZE    "flash_size"
#define _F_SYS_FEATURES      "features"
#define _F_SYS_RESET_REASON  "reset_reason"
#define _F_SYS_MAC           "mac"

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

/* Send */

static esp_err_t req_send_http_status(
    httpd_req_t *req, const char *status
) {
    const char *body = NULL;

    #ifdef CONFIG_EIF_LOG_ENABLE_WEB_SEND_TIMESTAMP
        if (strcmp(status, HTTPD_204) != 0) {
            char info_buf[32];

            uint32_t ts = esp_log_timestamp();
            snprintf(info_buf, sizeof(info_buf), "%u", ts);
            body = info_buf;
        }
    #endif

    CORE_LOG(D, "req_send_http_status:status - %s", status);
    httpd_resp_set_status(req, status);
    return httpd_resp_sendstr(req, body);
}

/* JSON */

static esp_err_t req_http_parse_json(httpd_req_t *req, cJSON **root) {
    const char *http_status = HTTPD_204;
    char *buffer = NULL;
    esp_err_t ret = ESP_OK;

    CHECK_CONDITION_WEB(req->content_len == 0, HTTPD_400, ESP_ERR_INVALID_ARG, 
        "Request body is empty, nothing to parse");

    buffer = malloc(req->content_len + 1);
    CHECK_CONDITION_WEB(!buffer, HTTPD_500, ESP_ERR_NO_MEM, 
        _ERR_ALLOCATE, req->content_len, "JSON buffer");

    int received = httpd_req_recv(req, buffer, req->content_len);
    CHECK_CONDITION_WEB((received <= 0), HTTPD_400, ESP_ERR_HTTPD_INVALID_REQ, 
        "HTTP receive failed (received: %d)", received);
    
    buffer[received] = '\0';

    *root = cJSON_Parse(buffer);
    CHECK_CONDITION_WEB(!*root, HTTPD_400, ESP_ERR_INVALID_STATE, _ERR_JSON_PARSE);
cleanup:
    if (ret != ESP_OK) {
        req_send_http_status(req, http_status);
        if (*root) {
            cJSON_Delete(*root);
            *root = NULL;
        }
    }
    if (buffer) free(buffer);

    return ret;
}

static esp_err_t req_json_get_field(
    httpd_req_t *req, cJSON *root, char *out_value,
    const char *field, size_t min_limit, size_t max_limit 
) {
    const char *http_status = HTTPD_204;
    esp_err_t ret = ESP_OK;

    cJSON *value = cJSON_GetObjectItemCaseSensitive(root, field);
    CHECK_CONDITION_WEB(
        !value || !cJSON_IsString(value) || !value->valuestring,
        HTTPD_400, ESP_ERR_INVALID_ARG, _ERR_JSON_MISSING, field);

    size_t len = strlen(value->valuestring);
    CHECK_CONDITION_WEB((len < min_limit || len >= max_limit),
        HTTPD_400, ESP_ERR_INVALID_ARG, 
        _ERR_INVALID_LEN, field, len, min_limit, max_limit - 1);

    strncpy(out_value, value->valuestring, max_limit - 1);
    out_value[max_limit - 1] = '\0';
cleanup:
    if (ret != ESP_OK) req_send_http_status(req, http_status);
    return ret;
}

static esp_err_t req_json_get_profile_index(
    httpd_req_t *req, cJSON *root, uint8_t *index
) {
    const char *http_status = HTTPD_204;
    esp_err_t ret = ESP_OK;
    const eif_t *cfg = eif_get();

    cJSON *value = cJSON_GetObjectItemCaseSensitive(
        root, _F_WIFI_PROF_IDX);
    CHECK_CONDITION_WEB(
        !value || !cJSON_IsNumber(value) || value->valueint < 0,
        HTTPD_400, ESP_ERR_INVALID_ARG, _ERR_JSON_MISSING, _F_WIFI_PROF_IDX);

    uint8_t result_index = (uint8_t)value->valueint;
    CHECK_CONDITION_WEB((result_index > cfg->wifi_profiles_count), 
        HTTPD_400, ESP_ERR_INVALID_ARG, _ERR_INVALID_IDX, 
        _F_WIFI_PROF_IDX, result_index, 0, cfg->wifi_profiles_count);

    *index = result_index;
cleanup:
    if (ret != ESP_OK) req_send_http_status(req, http_status);
    return ret;
}

/* Files */
static char hdr_cache_control_value[64] = "";
static esp_err_t set_cache(httpd_req_t *req, bool is_need) {

    #ifndef CONFIG_EIF_ENABLE_WEB_ADMIN_GUI
        (void)is_need;
    #else
        if (is_need) {
            #define ESP_WARN_CACHE_HIT 1234
            #define ETAG_VALUE "\"" __DATE__ " " __TIME__ "\""

            char if_none_match[64] = {0};
            if (httpd_req_get_hdr_value_str(
                req, "If-None-Match", if_none_match, sizeof(if_none_match)
            ) == ESP_OK) {
                if (strcmp(if_none_match, ETAG_VALUE) == 0) {
                    httpd_resp_set_status(req, HTTPD_304);
                    httpd_resp_send(req, NULL, 0);
                    return ESP_WARN_CACHE_HIT;
                }
            }
            httpd_resp_set_hdr(req, "ETag", ETAG_VALUE);
            httpd_resp_set_hdr(req, "Cache-Control", hdr_cache_control_value);
            return ESP_OK;
        }
    #endif

    httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate");
    httpd_resp_set_hdr(req, "Expires", "0");
    httpd_resp_set_hdr(req, "Connection", "close"); 
    httpd_resp_set_hdr(req, "Pragma", "no-cache");

    return ESP_OK;
}

#define RESP_TYPE_JS   "application/javascript; charset=UTF-8"
#define RESP_TYPE_JSON "application/json; charset=UTF-8"

#ifdef CONFIG_EIF_ENABLE_WEB_ADMIN_GUI
    #define RESP_TYPE_CSS  "text/css; charset=UTF-8"
    #define RESP_TYPE_HTML "text/html; charset=UTF-8"
    #define RESP_TYPE_TXT  "text/plain; charset=UTF-8"

    #ifdef CONFIG_EIF_ENABLE_BASIC_AUTH
        DEFINE_HTTP_FILE(e401_html,    e401_html_gz,    RESP_TYPE_HTML, false)
    #endif
    DEFINE_HTTP_FILE(e404_html,    e404_html_gz,    RESP_TYPE_HTML, false)
    DEFINE_HTTP_FILE(index_html,   index_html_gz,   RESP_TYPE_HTML, true)
    DEFINE_HTTP_FILE(network_html, network_html_gz, RESP_TYPE_HTML, true)
    DEFINE_HTTP_FILE(system_html,  system_html_gz,  RESP_TYPE_HTML, true)

    DEFINE_HTTP_FILE(style_css,    style_css_gz,    RESP_TYPE_CSS,  true)

    DEFINE_HTTP_FILE(license,      LICENSE_gz,      RESP_TYPE_TXT,  true)

    DEFINE_HTTP_FILE(json2_js,     json2_js_gz,     RESP_TYPE_JS,   true)
    DEFINE_HTTP_FILE(api_js,       api_js_gz,       RESP_TYPE_JS,   true)

    static esp_err_t httpd_err_404(httpd_req_t *req, httpd_err_code_t err) {
        CORE_LOG(W, "HTTP 404 error from '%s'", req->uri);
        httpd_resp_set_status(req, HTTPD_404);
        return h_file_e404_html(req); 
    }
#endif

/* WiFi */

static esp_err_t h_wifi_list_json(httpd_req_t *req) {
    const char *http_status = HTTPD_200;
    esp_err_t ret = ESP_OK;
    set_cache(req, false);

    const eif_t *cfg = eif_get();
    cJSON *root = NULL;
    cJSON *profiles = NULL;
    char *response_str = NULL;

    root = cJSON_CreateObject();
    CHECK_CONDITION_WEB(!root, 
        HTTPD_500, ESP_ERR_NO_MEM, _ERR_JSON_NO_MEM);

    cJSON_AddNumberToObject(root, 
        _F_WIFI_PROF_IDX_CUR, cfg->wifi_profile_index);

    wifi_ap_record_t info; 
    if (esp_wifi_sta_get_ap_info(&info) == ESP_OK) {
        cJSON_AddNumberToObject(root, _F_WIFI_RSSI_PROF, info.rssi);
    }
    
    profiles = cJSON_CreateArray();
    CHECK_CONDITION_WEB(!profiles, 
        HTTPD_500, ESP_ERR_NO_MEM, _ERR_JSON_NO_MEM);

    for (int i = 0; i <= cfg->wifi_profiles_count; i++) {
        cJSON *profile = cJSON_CreateObject();
        CHECK_CONDITION_WEB(!profile, 
            HTTPD_500, ESP_ERR_NO_MEM, _ERR_JSON_NO_MEM);

        char ssid[SSID_MAX_LEN + 1] = {0};
        char password[PASSWORD_MAX_LEN + 1] = {0};
        CHECK_ESP_ERR_T(E, nvs_wifi_profile_load(i, ssid, password), {}, {
            http_status = HTTPD_500;
            cJSON_Delete(profile);
            goto cleanup;
        }, _ERR_NVS_LOAD_PROF, i);

        cJSON_AddStringToObject(profile, _F_WIFI_SSID, ssid);
        if (i == 0) {
            cJSON_AddStringToObject(profile, _F_WIFI_PASS, password);
        }

        cJSON_AddItemToArray(profiles, profile);
    }

    cJSON_AddItemToObject(root, _F_WIFI_PROFS, profiles);

    response_str = cJSON_PrintUnformatted(root);
    CHECK_CONDITION_WEB(!response_str, 
        HTTPD_500, ESP_ERR_NO_MEM, _ERR_JSON_SER);

    httpd_resp_set_type(req, RESP_TYPE_JS);
    ret = httpd_resp_sendstr(req, response_str);

cleanup:
    if (response_str) free(response_str);
    if (root) {
        cJSON_Delete(root);
    } else if (profiles) { 
        cJSON_Delete(profiles);
    }

    if (ret != ESP_OK) req_send_http_status(req, http_status);
    return ret;
}

static esp_err_t h_wifi_update_do(httpd_req_t *req) {
    esp_err_t ret = ESP_OK;
    const char *http_status = HTTPD_204;
    set_cache(req, false);

    cJSON *root = NULL;

    CHECK_ESP_ERR_T(E, req_http_parse_json(req, &root), 
        {}, GOTO_CLEANUP_ERR(), _ERR_JSON_PARSE);

    uint8_t index = 0; 
    CHECK_ESP_ERR_T(E, req_json_get_profile_index(req, root, &index), 
        {}, GOTO_CLEANUP_ERR(), _ERR_NOT_FOUND_FIELD, _F_WIFI_PROF_IDX);

    char ssid[SSID_MAX_LEN + 1] = {0};
    CHECK_ESP_ERR_T(E, req_json_get_field(
        req, root, ssid, _F_WIFI_SSID, SSID_MIN_LEN, SSID_MAX_LEN
    ), {}, GOTO_CLEANUP_ERR(), _ERR_NOT_FOUND_FIELD, _F_WIFI_SSID);

    char password[PASSWORD_MAX_LEN + 1] = {0};
    CHECK_ESP_ERR_T(E, req_json_get_field(
        req, root, password, _F_WIFI_PASS, PASSWORD_MIN_LEN, PASSWORD_MAX_LEN
    ), {}, GOTO_CLEANUP_ERR(), _ERR_NOT_FOUND_FIELD, _F_WIFI_PASS);

    CHECK_ESP_ERR_T(E, nvs_wifi_profile_save(index, ssid, password), {}, {
        http_status = HTTPD_500;
        ret = err;
    }, _ERR_NVS_SAVE_PROF, index);

    req_send_http_status(req, http_status);
cleanup:
    if (root) cJSON_Delete(root);
    return ret;
}

static esp_err_t h_wifi_clear_do(httpd_req_t *req) {
    esp_err_t ret = ESP_OK;
    const char *http_status = HTTPD_204;
    set_cache(req, false);

    cJSON *root = NULL;
    CHECK_ESP_ERR_T(E, req_http_parse_json(req, &root), 
        {}, GOTO_CLEANUP_ERR(), _ERR_JSON_PARSE);

    uint8_t index = 0; 
    CHECK_ESP_ERR_T(E, req_json_get_profile_index(req, root, &index), 
        {}, GOTO_CLEANUP_ERR(), _ERR_NOT_FOUND_FIELD, _F_WIFI_PROF_IDX);

    CHECK_ESP_ERR_T(E, nvs_wifi_profile_save(
        index, WIFI_EMPTY_SSID, WIFI_EMPTY_PASS
    ), {}, {
        http_status = HTTPD_500;
        ret = err;
    }, _ERR_NVS_SAVE_PROF, index);

    req_send_http_status(req, http_status);
cleanup:
    if (root) cJSON_Delete(root);
    return ret;
}

static esp_err_t h_wifi_check_do(httpd_req_t *req) {
    esp_err_t ret = ESP_OK;
    const char *http_status = HTTPD_202;
    set_cache(req, false);

    cJSON *root = NULL;
    CHECK_ESP_ERR_T(E, req_http_parse_json(req, &root), 
        {}, GOTO_CLEANUP_ERR(), _ERR_JSON_PARSE);

    uint8_t index = 0; 
    CHECK_ESP_ERR_T(E, req_json_get_profile_index(req, root, &index), 
        {}, GOTO_CLEANUP_ERR(), _ERR_NOT_FOUND_FIELD, _F_WIFI_PROF_IDX);

    if (xTaskCreate(
        wifi_test_task, "wf_test", 1024 * 4, (void *)(uintptr_t)index, 5, NULL
    ) != pdPASS) {
        CORE_LOG(E, _ERR_SPAWN_TASK, "wifi_test_task", 
            (size_t)esp_get_free_heap_size());
        http_status = HTTPD_500;
        ret = ESP_ERR_NO_MEM;
    }

    if (ret == ESP_OK) {
        CORE_LOG(I, "WiFi test task launched for profile #%u", index);
    }
    req_send_http_status(req, http_status);
cleanup:
    if (root) cJSON_Delete(root);
    return ret;
}

static esp_err_t h_wifi_result_json(httpd_req_t *req) {
    esp_err_t ret = ESP_OK;
    const char *http_status = HTTPD_204;
    set_cache(req, false);

    const eif_t *cfg = eif_get();
    char *out_json = NULL;
    cJSON *res_obj = NULL;
    cJSON *root = NULL;

    CHECK_ESP_ERR_T(E, req_http_parse_json(req, &root), 
        {}, GOTO_CLEANUP_ERR(), _ERR_JSON_PARSE);

    uint8_t index = 0; 
    CHECK_ESP_ERR_T(E, req_json_get_profile_index(req, root, &index), 
        {}, GOTO_CLEANUP_ERR(), _ERR_NOT_FOUND_FIELD, _F_WIFI_PROF_IDX);
    
    res_obj = cJSON_CreateObject();
    CHECK_CONDITION_WEB(!res_obj, 
        HTTPD_500, ESP_ERR_NO_MEM, _ERR_JSON_NO_MEM);

    wifi_test_result res = cfg->wifi_result_tests[index];
    cJSON_AddBoolToObject(res_obj, _F_WIFI_RESULT, res.result);
    cJSON_AddNumberToObject(res_obj, _F_WIFI_RSSI, res.rssi);

    out_json = cJSON_PrintUnformatted(res_obj);
    CHECK_CONDITION_WEB(!out_json, 
        HTTPD_500, ESP_ERR_NO_MEM, _ERR_JSON_SER);
        
    httpd_resp_set_type(req, RESP_TYPE_JS);
    ret = httpd_resp_sendstr(req, out_json);

cleanup:
    if (strcmp(http_status, HTTPD_204) != 0) {
        req_send_http_status(req, http_status);
    }
    if (out_json) free(out_json);
    if (res_obj) cJSON_Delete(res_obj);
    if (root) cJSON_Delete(root);
    return ret;
}

/* TLS */

#ifdef CONFIG_EIF_ENABLE_TLS
    static esp_err_t h_tls_recreate_do(httpd_req_t *req) {
        esp_err_t ret = ESP_OK;
        const char *http_status = HTTPD_202;
        set_cache(req, false);

        int result = xTaskCreate(tls_recreate_task, "tls_gen", 1024 * 10, NULL, 5, NULL);
        CHECK_CONDITION_WEB(result != pdPASS, HTTPD_500, ESP_ERR_NO_MEM,
            _ERR_SPAWN_TASK, "tls_recreate_task", 
                (size_t)esp_get_free_heap_size());
    cleanup:
        req_send_http_status(req, http_status);
        return ret;
    }
#endif

/* System */

static esp_err_t h_sys_info_json(httpd_req_t *req) {
    esp_err_t ret = ESP_OK;
    const char *http_status = HTTPD_500; 
    set_cache(req, false);

    cJSON *root = NULL;
    char *json_str = NULL;

    esp_chip_info_t chip;
    esp_chip_info(&chip);
    
    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);

    root = cJSON_CreateObject();
    CHECK_CONDITION_WEB(!root, HTTPD_500, ESP_ERR_NO_MEM, _ERR_JSON_NO_MEM);

    cJSON_AddNumberToObject(root, _F_SYS_HEAP_FREE,   
        esp_get_free_heap_size());
    cJSON_AddNumberToObject(root, _F_SYS_HEAP_MIN,     
        esp_get_minimum_free_heap_size());
    cJSON_AddNumberToObject(root, _F_SYS_LARG_BLOCK,   
        heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));

    cJSON_AddNumberToObject(root, _F_SYS_UPTIME,       esp_timer_get_time() / 1000000);
    cJSON_AddNumberToObject(root, _F_SYS_CORES,        chip.cores);
    cJSON_AddNumberToObject(root, _F_SYS_CHIP_REV,     chip.revision);
    cJSON_AddNumberToObject(root, _F_SYS_FLASH_SIZE,   flash_size / (1024 * 1024));
    cJSON_AddNumberToObject(root, _F_SYS_CPU_FREQ,     EIF_CPU_FREQ_MHZ);
    cJSON_AddStringToObject(root, _F_SYS_CHIP_MODEL,   CONFIG_IDF_TARGET);
    cJSON_AddNumberToObject(root, _F_SYS_RESET_REASON, esp_rom_get_reset_reason(0));

    char feat[8];
    snprintf(feat, sizeof(feat), "%s%s%s", 
        (chip.features & CHIP_FEATURE_WIFI_BGN) ? "W" : "",
        (chip.features & CHIP_FEATURE_BT) ? "B" : "",
        (chip.features & CHIP_FEATURE_BLE) ? "L" : "");
    cJSON_AddStringToObject(root, _F_SYS_FEATURES, feat);

    uint8_t mac[6];
    char mac_str[18];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X", 
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    cJSON_AddStringToObject(root, _F_SYS_MAC, mac_str);

    json_str = cJSON_PrintUnformatted(root);
    CHECK_CONDITION_WEB(!json_str, HTTPD_500, ESP_ERR_NO_MEM, _ERR_JSON_SER);

    httpd_resp_set_type(req, RESP_TYPE_JSON);
    ret = httpd_resp_sendstr(req, json_str);
    goto cleanup_real;

cleanup:
    req_send_http_status(req, http_status);
cleanup_real:
    if (json_str) free(json_str);
    if (root) cJSON_Delete(root);
    return ret;
}

static esp_err_t h_sys_reboot_do(httpd_req_t *req) {
    esp_err_t ret = ESP_OK;
    const char *http_status = HTTPD_202;
    set_cache(req, false);

    int result = xTaskCreate(reboot_task, "reboot_task",
        CONFIG_EIF_REBOOT_TASK_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
    CHECK_CONDITION_WEB(result != pdPASS, HTTPD_500, ESP_ERR_NO_MEM,
        _ERR_SPAWN_TASK, "reboot_task", (size_t)esp_get_free_heap_size());
cleanup:
    req_send_http_status(req, http_status);
    return ret;
}

/* OTA */

static esp_err_t h_ota_info_json(httpd_req_t *req) {
    esp_err_t ret = ESP_OK;
    const char *http_status = HTTPD_204; 
    set_cache(req, false);

    cJSON *root = NULL;
    char *json_str = NULL;
    
    const esp_app_desc_t *app = esp_ota_get_app_description();
    const esp_partition_t *running = esp_ota_get_running_partition();
    
    char sha_str[65] = {0}; 
    esp_ota_get_app_elf_sha256(sha_str, sizeof(sha_str));

    root = cJSON_CreateObject();
    CHECK_CONDITION_WEB(!root, HTTPD_500, ESP_ERR_NO_MEM, _ERR_JSON_NO_MEM);

    esp_ota_img_states_t ota_state;
    const char* status_str = "unknown";
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        switch (ota_state) {
            case ESP_OTA_IMG_NEW:            status_str = "new"; break;
            case ESP_OTA_IMG_PENDING_VERIFY: status_str = "pending_verify"; break;
            case ESP_OTA_IMG_VALID:          status_str = "valid"; break;
            case ESP_OTA_IMG_INVALID:        status_str = "invalid"; break;
            case ESP_OTA_IMG_ABORTED:        status_str = "aborted"; break;
            default:                         status_str = "undefined"; break;
        }
    } else {
        status_str = "factory";
    }

    cJSON_AddStringToObject(root, _F_OTA_PROJECT,    app->project_name);
    cJSON_AddStringToObject(root, _F_OTA_VER,        app->version);
    cJSON_AddStringToObject(root, _F_OTA_BUILD_ID,   sha_str);
    cJSON_AddStringToObject(root, _F_OTA_BUILD_DATE, app->date);
    cJSON_AddStringToObject(root, _F_OTA_BUILD_TIME, app->time);
    cJSON_AddStringToObject(root, _F_OTA_IDF_VER,    app->idf_ver);
    cJSON_AddStringToObject(root, _F_OTA_GCC_VER,    __VERSION__);
    cJSON_AddStringToObject(root, _F_OTA_TARGET,     CONFIG_IDF_TARGET); 
    cJSON_AddStringToObject(root, _F_OTA_PARTITION,  running->label);
    cJSON_AddStringToObject(root, _F_OTA_STATUS,     status_str);

    json_str = cJSON_PrintUnformatted(root);
    CHECK_CONDITION_WEB(!json_str, HTTPD_500, ESP_ERR_NO_MEM, _ERR_JSON_SER);

    httpd_resp_set_type(req, RESP_TYPE_JSON);
    ret = httpd_resp_sendstr(req, json_str);
    goto cleanup_real;
cleanup:
    req_send_http_status(req, http_status);
cleanup_real:
    if (json_str) free(json_str);
    if (root) cJSON_Delete(root);
    return ret;
}

static bool is_ota_busy = false;
static esp_err_t h_ota_update_do(httpd_req_t *req) {
    esp_err_t ret = ESP_OK;
    const char *http_status = HTTPD_204;
    set_cache(req, false);

    if (is_ota_busy) {
        CORE_LOG(W, "OTA Update already in progress. Rejecting request.");
        req_send_http_status(req, HTTPD_409);
        return ESP_FAIL;
    }

    is_ota_busy = true;
    esp_ota_handle_t update_handle = 0;
    char *buf = NULL;
    const esp_partition_t *update_partition = 
        esp_ota_get_next_update_partition(NULL);

    CHECK_CONDITION_WEB(!update_partition, HTTPD_500, ESP_FAIL, 
        "No OTA partition");

    buf = malloc(CONFIG_EIF_WEB_SIZE_OTA_BUFFER);
    CHECK_CONDITION_WEB(!buf, HTTPD_500, ESP_ERR_NO_MEM, 
        _ERR_ALLOCATE, CONFIG_EIF_WEB_SIZE_OTA_BUFFER, "ota_buffer");

    size_t remaining = req->content_len;
    
    while (remaining > 0) {
        bool flag = remaining < CONFIG_EIF_WEB_SIZE_OTA_BUFFER;
        int received = httpd_req_recv(req, buf, 
            flag ? remaining : CONFIG_EIF_WEB_SIZE_OTA_BUFFER);
        if (received <= 0) {
            if (received == HTTPD_SOCK_ERR_TIMEOUT) continue;
            ret = ESP_FAIL; goto cleanup;
        }

        if (update_handle == 0) {
            CHECK_CONDITION_WEB((uint8_t)buf[0] != 0xE9, 
                HTTPD_400, ESP_ERR_INVALID_ARG, "Invalid magic byte");

            ret = esp_ota_begin(
                update_partition, OTA_SIZE_UNKNOWN, &update_handle);
            CHECK_CONDITION_WEB(ret != ESP_OK, HTTPD_500, ret, "Begin failed");
        }

        ret = esp_ota_write(update_handle, (const void *)buf, received);
        CHECK_CONDITION_WEB(ret != ESP_OK, HTTPD_500, ret, "Write failed");
            
        remaining -= received;
    }

    ret = esp_ota_end(update_handle);
    update_handle = 0;
    CHECK_CONDITION_WEB(ret != ESP_OK, HTTPD_500, ret, "End failed");

    ret = esp_ota_set_boot_partition(update_partition);
    CHECK_CONDITION_WEB(ret != ESP_OK, HTTPD_500, ret, "Boot err");

    req_send_http_status(req, http_status);
    
    if (buf) free(buf);
    int result = xTaskCreate(reboot_task, "reboot_task",
        CONFIG_EIF_REBOOT_TASK_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
    if (result != pdPASS) {
        CORE_LOG(E, _ERR_SPAWN_TASK, "reboot_task", 
            (size_t)esp_get_free_heap_size());
        is_ota_busy = false;
        ret = ESP_ERR_NO_MEM;
    }
    return ret;
cleanup:
    if (update_handle) esp_ota_abort(update_handle);
    if (buf) free(buf);
    req_send_http_status(req, http_status);
    is_ota_busy = false;
    return ret;
}

static esp_err_t h_ota_action_do(httpd_req_t *req) {
    set_cache(req, false);
    esp_err_t ret = ESP_OK;

    if (strstr(req->uri, "/_/ota/confirm.do")) {
        req_send_http_status(req, HTTPD_202);
        ret = esp_ota_mark_app_valid_cancel_rollback();
    } else if (strstr(req->uri, "/_/ota/rollback.do")) {
        req_send_http_status(req, HTTPD_202);
        int result = xTaskCreate(rollback_and_reboot_task, 
            "rollback_and_reboot_task", 4096, NULL, 5, NULL);
        if (result != pdPASS) {
            CORE_LOG(E, _ERR_SPAWN_TASK, "rollback_and_reboot_task", 
                (size_t)esp_get_free_heap_size());
            ret = ESP_ERR_NO_MEM;
        }
    } else {
        req_send_http_status(req, HTTPD_400);
        ret = ESP_FAIL;
    }

    return ret;
}

#ifdef CONFIG_EIF_ENABLE_BASIC_AUTH
    /* Admin Password */
    static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    static char *base64_encode(const unsigned char *input, size_t input_length) {
        size_t output_length = 4 * ((input_length + 2) / 3);
        char *encoded_data = malloc(output_length + 1);
        if (encoded_data == NULL) return NULL;

        for (size_t i = 0, j = 0; i < input_length; ) {
            uint32_t octet_a = i < input_length ? input[i++] : 0;
            uint32_t octet_b = i < input_length ? input[i++] : 0;
            uint32_t octet_c = i < input_length ? input[i++] : 0;
            uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

            encoded_data[j++] = base64_table[(triple >> 3 * 6) & 0x3F];
            encoded_data[j++] = base64_table[(triple >> 2 * 6) & 0x3F];
            encoded_data[j++] = base64_table[(triple >> 1 * 6) & 0x3F];
            encoded_data[j++] = base64_table[(triple >> 0 * 6) & 0x3F];
        }

        for (int i = 0; i < (3 - input_length % 3) % 3; i++) {
            encoded_data[output_length - 1 - i] = '=';
        }

        encoded_data[output_length] = '\0';
        return encoded_data;
    }

    static esp_err_t h_apass_update_do(httpd_req_t *req) {
        esp_err_t ret = ESP_OK;
        const char *http_status = HTTPD_204;
        cJSON *root = NULL;
        char *b64_pass = NULL;

        CHECK_ESP_ERR_T(E, req_http_parse_json(req, &root), 
            {}, GOTO_CLEANUP_ERR(), _ERR_JSON_PARSE);

        char pass[APASS_PASS_MAX_LEN + 1] = {0};
        CHECK_ESP_ERR_T(E, req_json_get_field(req, root, pass, 
            _F_APASS_PASS, APASS_PASS_MIN_LEN, APASS_PASS_MAX_LEN
        ), {}, GOTO_CLEANUP_ERR(), _ERR_NOT_FOUND_FIELD, _F_APASS_PASS);

        b64_pass = base64_encode((unsigned char *)pass, strlen(pass));
        if (b64_pass == NULL) {
            CORE_LOG(E, _ERR_BASE64_ENCODE, 0, (unsigned)strlen(pass));
            req_send_http_status(req, HTTPD_500);
            ret = ESP_ERR_INVALID_SIZE;
            goto cleanup;
        }

        char final_line[sizeof(AUTH_LINE_DEF) + AUTH_LINE_MAX_LEN + 1];
        snprintf(final_line, sizeof(final_line), "%s%s", AUTH_LINE_DEF, b64_pass);

        CORE_LOG(I, "Auth: %s", final_line);

        CHECK_ESP_ERR_T(E, nvs_auth_line_save(final_line), {}, {
            http_status = HTTPD_500;
            ret = err;
        }, _ERR_NVS_SAVE_APASS);

        req_send_http_status(req, http_status);
    cleanup:
        if (root) cJSON_Delete(root);
        if (b64_pass) free(b64_pass);
        return ret;
    }
#endif

/* Core */

static httpd_handle_t g_server_handle = NULL;
static const char* method_to_str(httpd_method_t method) {
    switch (method) {
        case HTTP_GET:     return "GET";
        case HTTP_POST:    return "POST";
        case HTTP_PUT:     return "PUT";
        case HTTP_DELETE:  return "DELETE";
        case HTTP_HEAD:    return "HEAD";
        case HTTP_OPTIONS: return "OPTIONS";
        case HTTP_PATCH:   return "PATCH";
        case HTTP_TRACE:   return "TRACE";
        case HTTP_CONNECT: return "CONNECT";
        default:           return "UNKNOWN";
    }
}

static esp_err_t req_check_auth_none(httpd_req_t *req) { return ESP_OK; }
DEFINE_MIDDLEWARE(public, req_check_auth_none);

#ifdef CONFIG_EIF_ENABLE_BASIC_AUTH
    static esp_err_t req_check_auth_admin(httpd_req_t *req) {
        esp_err_t ret = ESP_OK;

        /* +10 bytes to the buffer avoids password truncation. 
         * Example (imagine that 'AUTH_LINE_MAX_LEN' = 18):
         *   Header: Basic YWRtaW46MTIzc2Q=  (22 bytes)
         *   NVS:    Basic YWRtaW46MTIz      (18 bytes)
         * In this case, without +10 bytes, the data from the header
         * will be truncated to the length of the NVS and the comparison 
         * will be successful. */
        char buf[AUTH_LINE_MAX_LEN + 10];
        CHECK_ESP_ERR_T(E, httpd_req_get_hdr_value_str(
            req, "Authorization", buf, sizeof(buf)
        ), if (err == ESP_ERR_NOT_FOUND) goto unauthorized, {
            set_cache(req, false);
            req_send_http_status(req, HTTPD_400);
            return ret;
        }, "Auth header error");


        char current_pw[AUTH_LINE_MAX_LEN];
        CHECK_ESP_ERR_T(E, nvs_auth_line_load(current_pw), {}, {
            set_cache(req, false);
            req_send_http_status(req, HTTPD_500);
            return ret;
        }, "Failed to load auth data");

        if (strcmp(buf, current_pw) == 0) return ESP_OK;
    unauthorized:
        set_cache(req, false);
        CORE_LOG(W, "UNAUTHORIZED");

        #define REALM "Basic realm=\"" _F_AUTH_LINE "\""
        httpd_resp_set_hdr(req, "WWW-Authenticate", REALM);

        #ifdef CONFIG_EIF_ENABLE_WEB_ADMIN_GUI
            httpd_resp_set_status(req, HTTPD_401);
            return h_file_e401_html(req);
        #else
            return req_send_http_status(req, HTTPD_401);
        #endif
    }
    DEFINE_MIDDLEWARE(private, req_check_auth_admin);
#endif

static esp_err_t register_uris_with_middleware(
    const httpd_uri_t *routes, size_t route_count, esp_err_t (*mw_func)(httpd_req_t *)
) {
    esp_err_t ret = ESP_OK;
    if (route_count == 0) {
        CORE_LOG(W, "No handlers to register");
        return ESP_OK;
    }
    CHECK_NOT_NULL(routes, ESP_ERR_INVALID_ARG, goto cleanup);

    size_t max_method_len = 0, max_uri_len = 0;
    for (size_t i = 0; i < route_count; i++) {
        size_t method_len = strlen(method_to_str(routes[i].method));
        if (method_len > max_method_len) max_method_len = method_len;
        size_t uri_len = strlen(routes[i].uri);
        if (uri_len > max_uri_len) max_uri_len = uri_len;
    }

    for (size_t i = 0; i < route_count; i++) {
        httpd_uri_t route_to_reg = routes[i];
        
        route_to_reg.user_ctx = (void *)&routes[i]; 
        route_to_reg.handler = mw_func;

        httpd_register_uri_handler(g_server_handle, &route_to_reg);
        
        CORE_LOG(I, "Registered: %-*s %-*s HTTP/1.1", 
            (int)max_method_len, method_to_str(routes[i].method), 
            (int)max_uri_len, routes[i].uri);
    }
cleanup:
    return ret;
}

static const httpd_uri_t handlers[] = {
    #ifdef CONFIG_EIF_ENABLE_WEB_ADMIN_GUI
        /* ------------------------- Files ------------------------ */
        {"/_/files/license.txt",  HTTP_GET, h_file_license,      NULL},
        {"/_/files/index.html",   HTTP_GET, h_file_index_html,   NULL},
        {"/_/files/network.html", HTTP_GET, h_file_network_html, NULL},
        {"/_/files/system.html",  HTTP_GET, h_file_system_html,  NULL},
        {"/_/files/style.css",    HTTP_GET, h_file_style_css,    NULL},
        {"/_/files/json2.js",     HTTP_GET, h_file_json2_js,     NULL},
        {"/_/files/api.js",       HTTP_GET, h_file_api_js,       NULL},
    #endif

    /* ----------------------- WiFi API ---------------------- */
    /* Get parameters of all WiFi profiles */
    {"/_/wifi/list.json",   HTTP_GET,  h_wifi_list_json,    NULL},
    /* Update WiFi profile under index X */
    {"/_/wifi/update.do",   HTTP_POST, h_wifi_update_do,    NULL},
    /* Clear WiFi profile under index X */
    {"/_/wifi/clear.do",    HTTP_POST, h_wifi_clear_do,     NULL},
    /* Check WiFi availability using parameters under index X */
    {"/_/wifi/check.do",    HTTP_POST, h_wifi_check_do,     NULL},
    /* Getting the result from 'POST /_/wifi/check.do' */
    {"/_/wifi/result.json", HTTP_POST, h_wifi_result_json,  NULL},

    #ifdef CONFIG_EIF_ENABLE_TLS
        /* ----------------------- TLS API ---------------- ---- */
        /* Generate new TLS keys and certificate */
        {"/_/tls/recreate.do", HTTP_POST, h_tls_recreate_do, NULL},
    #endif

    /* ----------------------- System API ------------------------ */
    /* Getting information about the current system */
    {"/_/sys/info.json",    HTTP_GET,  h_sys_info_json,     NULL},
    /* Reboot system (ESP) */
    {"/_/sys/reboot.do",    HTTP_POST, h_sys_reboot_do,     NULL},

    /* ----------------------- OTA API ----------------------- */
    /* Getting information about the current firmware */
    {"/_/ota/info.json",    HTTP_GET,  h_ota_info_json,     NULL},
    /* Receiving new firmware */
    {"/_/ota/update.do",    HTTP_POST, h_ota_update_do,     NULL},
    /* Confirmation of a successful firmware update */
    {"/_/ota/confirm.do",   HTTP_POST, h_ota_action_do,     NULL},
    /* Rollback to previous firmware */
    {"/_/ota/rollback.do",  HTTP_POST, h_ota_action_do,     NULL},

    #ifdef CONFIG_EIF_ENABLE_BASIC_AUTH
        /* ---------------- Admin Password API ---------------- */
        /* Update Admin Password (for Web GUI) */
        {"/_/apass/update.do", HTTP_POST, h_apass_update_do, NULL},
    #endif
};

void eif_server_stop(void) {
    if (g_server_handle == NULL) {
        CORE_LOG(W, "Server is not running or already stopped");
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(100));
    #ifdef CONFIG_EIF_ENABLE_TLS
        httpd_ssl_stop(g_server_handle);
    #else 
        httpd_stop(g_server_handle);
    #endif
    g_server_handle = NULL;
    vTaskDelay(pdMS_TO_TICKS(100));
}

esp_err_t eif_server_launch(void) {
    CORE_LOG(I, "Starting server launch");

    if (g_server_handle != NULL) {
        CORE_LOG(W, "There is an old server: %p, must be deleted",
            g_server_handle);
        eif_server_stop();
        CORE_LOG(D, "Old server cleared: %p", g_server_handle);
    }

    #ifdef CONFIG_EIF_ENABLE_TLS
        CHECK_ESP_ERR_T(E, eif_set_tls_creds_from_nvs(), 
            {}, return err, "Failed to set TLS credentials");
    #endif

    const eif_t *cfg = eif_get();
    #ifdef CONFIG_EIF_ENABLE_TLS
        httpd_ssl_config_t conf_copy = cfg->server_config;
        CHECK_ESP_ERR_T(E, httpd_ssl_start(
            &g_server_handle, &conf_copy
        ), {}, return err, "httpd_ssl_start failed");
    #else
        httpd_config_t conf_copy = cfg->server_config;
        CHECK_ESP_ERR_T(E, httpd_start(
            &g_server_handle, &conf_copy
        ), {}, return err, "httpd_start failed");
    #endif

    CORE_LOG(I, "Server handle created: %p", g_server_handle);

    CORE_LOG(I, "Registering %d CORE URI handlers", 
        (int)(sizeof(handlers) / sizeof(handlers[0])));
    CHECK_ESP_ERR_T(E, register_uris_with_middleware(
        handlers, sizeof(handlers) / sizeof(handlers[0]), 
        #ifdef CONFIG_EIF_ENABLE_BASIC_AUTH
            middleware_private
        #else 
            middleware_public
        #endif
    ), {}, return err, "Failed to register CORE URIs");


    #ifdef CONFIG_EIF_ENABLE_WEB_ADMIN_GUI
        CHECK_ESP_ERR_T(E, httpd_register_err_handler(
            g_server_handle, HTTPD_404_NOT_FOUND, httpd_err_404
        ), {}, return err, "Failed to register handler '404 error'");
    #endif

    CORE_LOG(I, "Registering %d CLIENT URI handlers", cfg->uri_handlers_count);
    CHECK_ESP_ERR_T(E, register_uris_with_middleware(
        cfg->uri_handlers, cfg->uri_handlers_count, middleware_public
    ), {}, return err, "Failed to register CLIENT URIs");

    if (hdr_cache_control_value[0] == '\0') {
        #if defined(CONFIG_EIF_ENABLE_WEB_ADMIN_GUI) && defined(CONFIG_EIF_WEB_CACHE_MAX_AGE)
            snprintf(hdr_cache_control_value, sizeof(hdr_cache_control_value), 
                "public, max-age=%d", CONFIG_EIF_WEB_CACHE_MAX_AGE);
        #elif defined(CONFIG_EIF_ENABLE_WEB_ADMIN_GUI)
            snprintf(hdr_cache_control_value, sizeof(hdr_cache_control_value), 
                "public, max-age=360");
        #endif
    }

    #ifdef CONFIG_EIF_ENABLE_TLS
        CORE_LOG(I, "HTTPS Server launched successfully");
    #else
        CORE_LOG(I, "HTTP Server launched successfully");
    #endif

    return ESP_OK;
}