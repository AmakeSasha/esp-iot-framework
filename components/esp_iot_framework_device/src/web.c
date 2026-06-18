/* SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Library: esp_iot_framework_device
 * Folder: ./components/esp_iot_framework_device/src
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

#include "sdkconfig.h"

#include "cJSON.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "esp_flash.h"
#include "esp_timer.h"
#include "esp_ota_ops.h"
#include "esp_chip_info.h"
#include "esp_app_format.h"
#include "esp_idf_version.h"
#ifdef CONFIG_EIF_ENABLE_TLS
    #include "esp_https_server.h"
#else
    #include "esp_http_server.h"
#endif

#include "device_macros.h"
#include "device_internal.h"
#include "esp_iot_framework_device.h"
#include "esp_iot_framework_core_ext.h"
#include "esp_iot_framework_core_macros.h"

#ifdef CONFIG_EIF_ENABLE_TLS
    #define TAG "HTTPS server"
#else
    #define TAG "HTTP server"
#endif


/* --- */

#define SERVER_ERR_JSON_NO_MEM     "Failed to create JSON root"
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
#define SERVER_ERR_STR_FORMAT      "String formatting truncated or failed"

#define FIELD_WIFI_PROF_IDX     "profile_index"
#define FIELD_WIFI_PROF_IDX_CUR "current_profile_index"
#define FIELD_WIFI_SSID         "ssid"
#define FIELD_WIFI_PASS         "password"
#define FIELD_WIFI_RSSI         "rssi"
#define FIELD_WIFI_RSSI_PROF    "rssi_now_profile"
#define FIELD_WIFI_PROFS        "profiles"
#define FIELD_WIFI_RESULT       "result"

#define FIELD_APASS_PASS        "password"

#define FIELD_OTA_PROJECT       "project"
#define FIELD_OTA_VER           "version"
#define FIELD_OTA_BUILD_ID      "build_id"
#define FIELD_OTA_BUILD_DATE    "build_date"
#define FIELD_OTA_BUILD_TIME    "build_time"
#define FIELD_OTA_IDF_VER       "idf_version"
#define FIELD_OTA_GCC_VER       "compiler"
#define FIELD_OTA_TARGET        "target"
#define FIELD_OTA_PARTITION     "partition"
#define FIELD_OTA_STATUS        "ota_status"

#define FIELD_SYS_FEATURES      "features"
#define FIELD_SYS_FEATURES_WIFI "has_wifi"
#define FIELD_SYS_FEATURES_BT   "has_bluetooth"
#define FIELD_SYS_FEATURES_BLE  "has_ble"
#define FIELD_SYS_HEAP_FREE     "heap_free"
#define FIELD_SYS_HEAP_MIN      "heap_min"
#define FIELD_SYS_LARG_BLOCK    "largest_block"
#define FIELD_SYS_UPTIME        "uptime"
#define FIELD_SYS_CPU_FREQ      "cpu_freq"
#define FIELD_SYS_CORES         "cores"
#define FIELD_SYS_CHIP_MODEL    "chip_model"
#define FIELD_SYS_CHIP_REV      "chip_rev"
#define FIELD_SYS_FLASH_SIZE    "flash_size"
#define FIELD_SYS_RESET_REASON  "reset_reason"
#define FIELD_SYS_MAC           "mac"

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

#define RESP_TYPE_JS   "application/javascript; charset=UTF-8"
#define RESP_TYPE_JSON "application/json; charset=UTF-8"

#ifdef CONFIG_EIF_ENABLE_WEB_ADMIN_GUI
    #define RESP_TYPE_CSS  "text/css; charset=UTF-8"
    #define RESP_TYPE_HTML "text/html; charset=UTF-8"
    #define RESP_TYPE_TXT  "text/plain; charset=UTF-8"
    #define RESP_TYPE_PNG  "image/png"

    #define ESP_WARN_CACHE_HIT 1234
    #define ETAG_VALUE "\"" __DATE__ " " __TIME__ "\""
#endif

#ifdef CONFIG_EIF_ENABLE_BASIC_AUTH
    #define HTTP_BASIC_AUTH_REALM "Basic realm=\"web_basic_auth\""
#endif
#define HTTP_METHOD_MAX_LEN 8
#define HTTP_URI_MAX_LEN 16 * 1024

#define HDR_CACHE_CONTROL_VALUE "public, max-age=" EIF_STR(CONFIG_EIF_WEB_CACHE_MAX_AGE)

/* Send */

#define BUFFER_TIMESTAMP 16U

static void httpd_resp_sendstatus(
    httpd_req_t * const req, const char * const status
) {
    #ifdef CONFIG_EIF_LOG_ENABLE_WEB_SEND_TIMESTAMP
        char body_buf[BUFFER_TIMESTAMP] = {0};
    #endif
    const char * p_body = "0";

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

static esp_err_t req_http_parse_json(
    httpd_req_t * const req, cJSON * * const root
) {
    esp_err_t ret = ESP_OK;

    int received = 0;
    char * buffer = NULL;

    EIF_IF_OK_CHECK_CONDITION(ret, (req->content_len == 0),
        ESP_ERR_INVALID_ARG, "Request body is empty, nothing to parse");

    if (ret == ESP_OK) {
        buffer = (char *)pvPortMalloc(req->content_len + 1);
        EIF_IF_OK_CHECK_CONDITION(ret, (buffer == NULL), ESP_ERR_NO_MEM,
            SERVER_ERR_ALLOCATE, req->content_len, "JSON buffer");
    }

    if (ret == ESP_OK) {
        received = httpd_req_recv(req, buffer, req->content_len);
        EIF_IF_OK_CHECK_CONDITION(ret,
            (received <= 0), ESP_ERR_HTTPD_INVALID_REQ,
            "HTTP receive failed (received: %d)", received);
    }


    if (ret == ESP_OK) {
        buffer[received] = '\0';

        *root = cJSON_Parse(buffer);
        EIF_IF_OK_CHECK_CONDITION(ret, *root == NULL,
            ESP_ERR_INVALID_STATE, SERVER_ERR_JSON_PARSE);
    }

    /* Cleanup */
    if (ret != ESP_OK) {
        if (*root != NULL) {
            cJSON_Delete(*root);
            *root = NULL;
        }
    }
    if (buffer != NULL) {
        vPortFree(buffer);
    }

    return ret;
}

static esp_err_t req_json_get_field(
    const cJSON * const root, char * const out_value,
    const char * const field, const size_t min_limit, const size_t max_limit
) {
    esp_err_t ret = ESP_OK;

    const cJSON * const value = cJSON_GetObjectItemCaseSensitive(root, field);
    EIF_IF_OK_CHECK_CONDITION(ret,
        ((value == NULL) || !cJSON_IsString(value) || (value->valuestring == NULL)),
        ESP_ERR_INVALID_ARG, SERVER_ERR_JSON_MISSING, field);

    /* @note The second condition is necessary to successfully pass the
     * CppCheck check. */
    if ((ret == ESP_OK) && (value != NULL)) {
        const size_t len = eif_strnlen(value->valuestring, max_limit);
        EIF_IF_OK_CHECK_CONDITION(ret,
            ((len < min_limit) || (len >= max_limit)), ESP_ERR_INVALID_ARG,
            SERVER_ERR_INVALID_LEN, field, len, min_limit, max_limit - 1U);
    }

    /* @note The second condition is necessary to successfully pass the
     * CppCheck check. */
    if ((ret == ESP_OK) && (value != NULL)) {
        (void)strncpy(out_value, value->valuestring, max_limit - 1U);
        out_value[max_limit - 1U] = '\0';
    }

    /* Cleanup */
    return ret;
}

static esp_err_t req_json_get_profile_index(
    const cJSON * const root, uint8_t * const index
) {
    esp_err_t ret = ESP_OK;

    const cJSON * value = NULL;
    const uint8_t wifi_profiles_count = eif_wifi_get_profiles_count();

    value = cJSON_GetObjectItemCaseSensitive(root, FIELD_WIFI_PROF_IDX);
    if ((value == NULL) || !cJSON_IsNumber(value) || (value->valueint < 0)) {
        ret = ESP_ERR_INVALID_ARG;
        EIF_LOG_E(SERVER_ERR_JSON_MISSING, FIELD_WIFI_PROF_IDX);
    }

    /* @note The second condition is necessary to successfully pass the
     * CppCheck check. */
    if ((ret == ESP_OK) && (value != NULL)) {   
        if ((value->valueint > wifi_profiles_count) || (value->valueint > 255)) {
            ret = ESP_ERR_INVALID_ARG;
            EIF_LOG_E(SERVER_ERR_INVALID_IDX,
                FIELD_WIFI_PROF_IDX, value->valueint, 0, wifi_profiles_count);
        } else {
            *index = (uint8_t)value->valueint;
        }
    }

    /* Cleanup */
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
    typedef struct {
        const uint8_t * const start;
        const uint8_t * const end;
        const char * const content_type;
        const char * const file_name;
        bool need_cache;
    } eif_web_file_t;

    static esp_err_t httpd_resp_sendfile(
        httpd_req_t * const req, const eif_web_file_t * const file
    ) {
        esp_err_t ret = ESP_OK;
        const size_t size = (file->end - file->start);

        EIF_IF_OK_CHECK_ESP_ERR_T(ret, set_cache(req, file->need_cache),
            "Failed to set cache for '%s'", file->file_name);

        if (ret == ESP_OK) {
            httpd_resp_set_type(req, file->content_type);
            httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
            return httpd_resp_send(req, (const char *)file->start, size);
        } else if (ret == ESP_WARN_CACHE_HIT) {
            ret = ESP_OK;
        } else { ; }

        /* Cleanup */
        return ret;
    }

    #ifdef CONFIG_EIF_ENABLE_BASIC_AUTH
        EIF_DEFINE_HTTP_FILE(e401_html_gz, RESP_TYPE_HTML, false)
    #endif
    EIF_DEFINE_HTTP_FILE(e404_html_gz,    RESP_TYPE_HTML, false)
    EIF_DEFINE_HTTP_FILE(index_html_gz,   RESP_TYPE_HTML, true)
    EIF_DEFINE_HTTP_FILE(network_html_gz, RESP_TYPE_HTML, true)
    EIF_DEFINE_HTTP_FILE(system_html_gz,  RESP_TYPE_HTML, true)

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



/* WiFi */

static esp_err_t h_wifi_list_json(httpd_req_t * const req) {
    esp_err_t ret = ESP_OK;

    cJSON * root = NULL;
    cJSON * profile = NULL;
    cJSON * profiles = NULL;
    char * response_str = NULL;
    wifi_ap_record_t info = {0};
    char ssid[EIF_WIFI_SSID_MAX_LEN] = {0};
    char pass[EIF_WIFI_PASS_MAX_LEN] = {0};
    const uint8_t wifi_profiles_count = eif_wifi_get_profiles_count();
    const uint8_t wifi_profiles_index = eif_wifi_get_current_profile_index();

    (void)set_cache(req, false);

    root = cJSON_CreateObject();
    EIF_IF_OK_CHECK_CONDITION(ret, root == NULL,
        ESP_ERR_NO_MEM, SERVER_ERR_JSON_NO_MEM);

    if (ret == ESP_OK) {
        profiles = cJSON_CreateArray();
        EIF_IF_OK_CHECK_CONDITION(ret, profiles == NULL,
            ESP_ERR_NO_MEM, SERVER_ERR_JSON_NO_MEM);
    }

    for (uint16_t idx = 0U; idx <= (uint16_t)wifi_profiles_count; idx++) {
        if (ret == ESP_OK) {
            profile = cJSON_CreateObject();
            EIF_IF_OK_CHECK_CONDITION(ret, profile == NULL,
                ESP_ERR_NO_MEM, SERVER_ERR_JSON_NO_MEM);

            EIF_IF_OK_CHECK_ESP_ERR_T(ret,
                eif_nvs_wifi_profile_load(idx, ssid, pass),
                SERVER_ERR_NVS_LOAD_PROF, idx);

            if (ret != ESP_OK) {
                cJSON_Delete(profile);
                profile = NULL;
                cJSON_Delete(profiles);
                profiles = NULL;
            } else {
                cJSON_AddStringToObject(profile, FIELD_WIFI_SSID, ssid);
                if (idx == 0U) {
                    cJSON_AddStringToObject(profile, FIELD_WIFI_PASS, pass);
                }

                cJSON_AddItemToArray(profiles, profile);
            }
        }
    }

    if (ret == ESP_OK) {
        cJSON_AddItemToObject(root, FIELD_WIFI_PROFS, profiles);
        cJSON_AddNumberToObject(root,
            FIELD_WIFI_PROF_IDX_CUR, wifi_profiles_index);

        if (esp_wifi_sta_get_ap_info(&info) == ESP_OK) {
            cJSON_AddNumberToObject(root, FIELD_WIFI_RSSI_PROF, info.rssi);
        }

        response_str = cJSON_PrintUnformatted(root);
        EIF_IF_OK_CHECK_CONDITION(ret, response_str == NULL,
            ESP_ERR_NO_MEM, SERVER_ERR_JSON_SER);
    }

    if (ret == ESP_OK) {
        httpd_resp_set_type(req, RESP_TYPE_JSON);
        httpd_resp_sendstr(req, response_str);
    }

    /* Cleanup */
    if (response_str != NULL) {
        cJSON_free(response_str);
    }
    if (root != NULL) {
        cJSON_Delete(root);
    } else if (profiles != NULL) {
        cJSON_Delete(profiles);
    } else { ; }

    if (ret != ESP_OK) {
        httpd_resp_sendstatus(req, HTTPD_500);
    }

    return ret;
}

static esp_err_t h_wifi_update_do(httpd_req_t * const req) {
    esp_err_t ret = ESP_OK;

    uint8_t index = 0;
    cJSON * root = NULL;
    char ssid[EIF_WIFI_SSID_MAX_LEN] = {0};
    char pass[EIF_WIFI_PASS_MAX_LEN] = {0};

    (void)set_cache(req, false);

    EIF_IF_OK_CHECK_ESP_ERR_T(ret,
        req_http_parse_json(req, &root), SERVER_ERR_JSON_PARSE);

    EIF_IF_OK_CHECK_ESP_ERR_T(ret,
        req_json_get_profile_index(root, &index),
        SERVER_ERR_NOT_FOUND_FIELD, FIELD_WIFI_PROF_IDX);

    EIF_IF_OK_CHECK_ESP_ERR_T(ret, req_json_get_field(root,
        ssid, FIELD_WIFI_SSID, EIF_WIFI_SSID_MIN_LEN, EIF_WIFI_SSID_MAX_LEN
    ), SERVER_ERR_NOT_FOUND_FIELD, FIELD_WIFI_SSID);

    EIF_IF_OK_CHECK_ESP_ERR_T(ret,req_json_get_field(root,
        pass, FIELD_WIFI_PASS, EIF_WIFI_PASS_MIN_LEN, EIF_WIFI_PASS_MAX_LEN
    ), SERVER_ERR_NOT_FOUND_FIELD, FIELD_WIFI_PASS);

    if (ret == ESP_ERR_NO_MEM) {
        httpd_resp_sendstatus(req, HTTPD_500);
    } else if (ret != ESP_OK) {
        httpd_resp_sendstatus(req, HTTPD_400);
    } else { /* ret == ESP_OK */
        EIF_IF_OK_CHECK_ESP_ERR_T(ret,
            eif_nvs_wifi_profile_save(index, ssid, pass),
            SERVER_ERR_NVS_SAVE_PROF, index);

        if (ret != ESP_OK) {
            httpd_resp_sendstatus(req, HTTPD_500);
        }
    }

    /* Cleanup */
    if (root != NULL) {
        cJSON_Delete(root);
    }
    if (ret == ESP_OK) {
        httpd_resp_sendstatus(req, HTTPD_204);
    }
    return ret;
}

static esp_err_t h_wifi_clear_do(httpd_req_t * const req) {
    esp_err_t ret = ESP_OK;

    uint8_t index = 0;
    cJSON * root = NULL;

    (void)set_cache(req, false);

    EIF_IF_OK_CHECK_ESP_ERR_T(ret, req_http_parse_json(req, &root),
        SERVER_ERR_JSON_PARSE);
    EIF_IF_OK_CHECK_ESP_ERR_T(ret, req_json_get_profile_index(root, &index),
        SERVER_ERR_NOT_FOUND_FIELD, FIELD_WIFI_PROF_IDX);

    if (ret != ESP_OK) {
        httpd_resp_sendstatus(req, HTTPD_400);
    } else {
        EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_nvs_wifi_profile_save(index, "", ""),
            SERVER_ERR_NVS_SAVE_PROF, index);

        if (ret != ESP_OK) {
            httpd_resp_sendstatus(req, HTTPD_500);
        }
    }

    /* Cleanup */
    if (root != NULL) {
        cJSON_Delete(root);
    }
    if (ret == ESP_OK) {
        httpd_resp_sendstatus(req, HTTPD_204);
    }
    return ret;
}

static esp_err_t h_wifi_check_do(httpd_req_t * const req) {
    esp_err_t ret = ESP_OK;

    uint8_t index = 0;
    cJSON * root = NULL;

    (void)set_cache(req, false);

    EIF_IF_OK_CHECK_ESP_ERR_T(ret, req_http_parse_json(req, &root),
        SERVER_ERR_JSON_PARSE);
    EIF_IF_OK_CHECK_ESP_ERR_T(ret, req_json_get_profile_index(root, &index),
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
    if (root != NULL) {
        cJSON_Delete(root);
    }
    return ret;
}

static esp_err_t h_wifi_result_json(httpd_req_t * const req) {
    esp_err_t ret = ESP_OK;

    uint8_t index = 0;
    cJSON * root = NULL;
    char * out_json = NULL;
    cJSON * res_obj = NULL;
    eif_wifi_test_result test_res = {0};

    (void)set_cache(req, false);

    EIF_IF_OK_CHECK_ESP_ERR_T(ret, req_http_parse_json(req, &root),
        SERVER_ERR_JSON_PARSE);
    EIF_IF_OK_CHECK_ESP_ERR_T(ret, req_json_get_profile_index(root, &index),
        SERVER_ERR_NOT_FOUND_FIELD, FIELD_WIFI_PROF_IDX);


    if (ret != ESP_OK) {
        httpd_resp_sendstatus(req, HTTPD_400);
    } else {
        res_obj = cJSON_CreateObject();
        EIF_IF_OK_CHECK_CONDITION(ret, res_obj == NULL,
            ESP_ERR_NO_MEM, SERVER_ERR_JSON_NO_MEM);

        EIF_IF_OK_CHECK_ESP_ERR_T(ret,
            eif_wifi_get_test_result(index, &test_res),
            "Couldn't upload Wi-Fi profile test results");

        if (ret == ESP_OK) {
            cJSON_AddBoolToObject(res_obj,
                FIELD_WIFI_RESULT, test_res.connected);
            cJSON_AddNumberToObject(res_obj,
                FIELD_WIFI_RSSI, test_res.rssi);

            out_json = cJSON_PrintUnformatted(res_obj);
            EIF_IF_OK_CHECK_CONDITION(ret, out_json == NULL,
                ESP_ERR_NO_MEM, SERVER_ERR_JSON_SER);
        }

        if (ret == ESP_OK) {
            httpd_resp_set_type(req, RESP_TYPE_JSON);
            ret = httpd_resp_sendstr(req, out_json);
        } else {
            httpd_resp_sendstatus(req, HTTPD_500);
        }
    }

    /* Cleanup */
    if (out_json != NULL) {
        cJSON_free(out_json);
    }
    if (res_obj != NULL) {
        cJSON_Delete(res_obj);
    }
    if (root != NULL) {
        cJSON_Delete(root);
    }
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

    cJSON * root = NULL;
    uint8_t mac[6] = {0};
    char mac_str[18] = {0};
    uint32_t flash_size = 0;
    cJSON * features = NULL;
    esp_chip_info_t chip = {0};
    char * response_str = NULL;

    (void)set_cache(req, false);

    root = cJSON_CreateObject();
    EIF_IF_OK_CHECK_CONDITION(ret, root == NULL, ESP_ERR_NO_MEM,
        SERVER_ERR_JSON_NO_MEM);

    if (ret == ESP_OK) {
        features = cJSON_CreateObject();
        EIF_IF_OK_CHECK_CONDITION(ret, features == NULL, ESP_ERR_NO_MEM,
            SERVER_ERR_JSON_NO_MEM);
    }

    if (ret == ESP_OK) {
        esp_chip_info(&chip);
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        esp_flash_get_size(NULL, &flash_size);

        cJSON_AddBoolToObject(features, FIELD_SYS_FEATURES_WIFI,
            (chip.features & CHIP_FEATURE_WIFI_BGN));
        cJSON_AddBoolToObject(features, FIELD_SYS_FEATURES_BT,
            (chip.features & CHIP_FEATURE_BT));
        cJSON_AddBoolToObject(features, FIELD_SYS_FEATURES_BLE,
            (chip.features & CHIP_FEATURE_BLE));
        cJSON_AddItemToObject(root, FIELD_SYS_FEATURES, features);

        cJSON_AddNumberToObject(root, FIELD_SYS_HEAP_FREE,  
            esp_get_free_heap_size());
        cJSON_AddNumberToObject(root, FIELD_SYS_HEAP_MIN,    
            esp_get_minimum_free_heap_size());
        cJSON_AddNumberToObject(root, FIELD_SYS_LARG_BLOCK,  
            heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
        cJSON_AddNumberToObject(root, FIELD_SYS_UPTIME,
            esp_timer_get_time() / 1000000ULL);
        cJSON_AddNumberToObject(root, FIELD_SYS_CORES,
            chip.cores);
        cJSON_AddNumberToObject(root, FIELD_SYS_CHIP_REV,
            chip.revision);
        cJSON_AddNumberToObject(root, FIELD_SYS_FLASH_SIZE,
            flash_size / (1024U * 1024U));
        cJSON_AddNumberToObject(root, FIELD_SYS_CPU_FREQ,
            EIF_CPU_FREQ_MHZ);
        cJSON_AddStringToObject(root, FIELD_SYS_CHIP_MODEL,
            CONFIG_IDF_TARGET);
        cJSON_AddNumberToObject(root, FIELD_SYS_RESET_REASON,
            (double)esp_rom_get_reset_reason(0));

        int res = snprintf(mac_str, sizeof(mac_str),
            "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        EIF_IF_OK_CHECK_CONDITION(ret,
            (res < 0) || (res >= (int)sizeof(mac_str)),
            ESP_ERR_INVALID_SIZE, SERVER_ERR_STR_FORMAT);
    }

    if (ret == ESP_OK) {
        cJSON_AddStringToObject(root, FIELD_SYS_MAC, mac_str);

        response_str = cJSON_PrintUnformatted(root);
        EIF_IF_OK_CHECK_CONDITION(ret, response_str == NULL,
            ESP_ERR_NO_MEM, SERVER_ERR_JSON_SER);
    }

    if (ret == ESP_OK) {
        httpd_resp_set_type(req, RESP_TYPE_JSON);
        httpd_resp_sendstr(req, response_str);
    }

    /* Cleanup */
    if (ret != ESP_OK) {
        httpd_resp_sendstatus(req, HTTPD_500);
    }
    if (root != NULL) {
        cJSON_Delete(root);
    } else if (features != NULL) {
        cJSON_Delete(features);
    } else { ; }
    if (response_str != NULL) {
        cJSON_free(response_str);
    }
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



/* OTA */

static esp_err_t h_ota_info_json(httpd_req_t * const req) {
    esp_err_t ret = ESP_OK;

    cJSON * root = NULL;
    char sha_str[65] = {0};
    char * response_str = NULL;
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

    root = cJSON_CreateObject();
    EIF_IF_OK_CHECK_CONDITION(ret, root == NULL, ESP_ERR_NO_MEM,
        SERVER_ERR_JSON_NO_MEM);

    if (ret == ESP_OK) {
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

        cJSON_AddStringToObject(root, FIELD_OTA_PROJECT,    app->project_name);
        cJSON_AddStringToObject(root, FIELD_OTA_VER,        app->version);
        cJSON_AddStringToObject(root, FIELD_OTA_BUILD_ID,   sha_str);
        cJSON_AddStringToObject(root, FIELD_OTA_BUILD_DATE, app->date);
        cJSON_AddStringToObject(root, FIELD_OTA_BUILD_TIME, app->time);
        cJSON_AddStringToObject(root, FIELD_OTA_IDF_VER,    app->idf_ver);
        cJSON_AddStringToObject(root, FIELD_OTA_GCC_VER,    __VERSION__);
        cJSON_AddStringToObject(root, FIELD_OTA_TARGET,     CONFIG_IDF_TARGET);
        cJSON_AddStringToObject(root, FIELD_OTA_PARTITION,  running->label);
        cJSON_AddStringToObject(root, FIELD_OTA_STATUS,     status_str);

        response_str = cJSON_PrintUnformatted(root);
        EIF_IF_OK_CHECK_CONDITION(ret, response_str == NULL, ESP_ERR_NO_MEM,
            SERVER_ERR_JSON_SER);
    }
    if (ret == ESP_OK) {
        httpd_resp_set_type(req, RESP_TYPE_JSON);
        httpd_resp_sendstr(req, response_str);
    }

    /* Cleanup */
    if (ret != ESP_OK) {
        httpd_resp_sendstatus(req, HTTPD_500);
    }
    if (response_str != NULL) {
        cJSON_free(response_str);
    }
    if (root != NULL) {
        cJSON_Delete(root);
    }
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
        buf = pvPortMalloc(CONFIG_EIF_WEB_SIZE_OTA_BUFFER);
        EIF_IF_OK_CHECK_CONDITION(ret, buf == NULL, ESP_ERR_NO_MEM,
            SERVER_ERR_ALLOCATE, CONFIG_EIF_WEB_SIZE_OTA_BUFFER, "ota_buffer");
    }
       
    while ((remaining > 0U) && (ret == ESP_OK)) {
        const bool flag = remaining < CONFIG_EIF_WEB_SIZE_OTA_BUFFER;
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

        cJSON * root = NULL;
        char pass[EIF_BASIC_AUTH_PASS_MAX_LEN] = {0};

        EIF_IF_OK_CHECK_ESP_ERR_T(ret,
            req_http_parse_json(req, &root), SERVER_ERR_JSON_PARSE);

        EIF_IF_OK_CHECK_ESP_ERR_T(ret, req_json_get_field(
            root, pass, FIELD_APASS_PASS,
            EIF_BASIC_AUTH_PASS_MIN_LEN, EIF_BASIC_AUTH_PASS_MAX_LEN
        ), SERVER_ERR_NOT_FOUND_FIELD, FIELD_APASS_PASS);

        if (ret != ESP_OK) {
            httpd_resp_sendstatus(req, HTTPD_400);
        } else {
            EIF_IF_OK_CHECK_ESP_ERR_T(ret,
                eif_nvs_basic_auth_line_save((uint8_t *)pass),
                SERVER_ERR_NVS_SAVE_APASS);

            if (ret != ESP_OK) {
                httpd_resp_sendstatus(req, HTTPD_500);
            }
        }

        /* Cleanup */
        if (ret == ESP_OK) {
            httpd_resp_sendstatus(req, HTTPD_204);
        }
        if (root != NULL) {
            cJSON_Delete(root);
        }
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


    size_t max_method_len = 0;
    size_t max_uri_len = 0;

    if (route_count == 0U) {
        EIF_LOG_W("No handlers to register");
    } else {
        EIF_IF_OK_CHECK_NOT_NULL(ret, routes, ESP_ERR_INVALID_ARG);

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
                {"/favicon.ico",          HTTP_GET, sendf_logo_png_gz,     NULL},
            #endif
            {"/_/files/license.txt",  HTTP_GET, sendf_LICENSE_gz,      NULL},
            {"/_/files/index.html",   HTTP_GET, sendf_index_html_gz,   NULL},
            {"/_/files/network.html", HTTP_GET, sendf_network_html_gz, NULL},
            {"/_/files/system.html",  HTTP_GET, sendf_system_html_gz,  NULL},
            {"/_/files/style.css",    HTTP_GET, sendf_style_css_gz,    NULL},
            {"/_/files/json2.js",     HTTP_GET, sendf_json2_js_gz,     NULL},
            {"/_/files/api.js",       HTTP_GET, sendf_api_js_gz,       NULL},
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