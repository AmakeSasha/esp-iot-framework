/*
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp_iot_framework
 * Folder: src
 * File: config.c
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

#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "sdkconfig.h"
#include "esp_ota_ops.h"
#ifdef CONFIG_EIF_ENABLE_TLS
    #include "esp_https_server.h"
#else
    #include "esp_http_server.h"
#endif

#include "macros.h"
#include "core_internal.h"

#define TAG "Global config"

/* --- */

#define _ERR_INVALID_LEN     "Invalid '%s' length: %u (allowed range: %u-%u)"

#define _MSG_CALL_SETTER  "Calling the setter `%s`"
#define _MSG_CALL_UPDATER "Calling the setter `%s`"
#define _MSG_REPLACED     "`%s` with %d replaced by %d"

/* --- */

static eif_t cfg;

void eif_initialize(void) {
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;
    
    if (esp_ota_get_state_partition(running, &state) == ESP_OK) {
        CORE_LOG(I, "Partition state: %d", state);
    } else {
        CORE_LOG(I, "Partition state: unknown");
    }

    #ifdef CONFIG_EIF_ENABLE_TLS
        httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();
        config.cacert_pem = NULL;
        config.cacert_len = 0;
        config.prvtkey_pem = NULL;
        config.prvtkey_len = 0;
        config.session_tickets = false;
        config.transport_mode = HTTPD_SSL_TRANSPORT_SECURE;
        config.httpd.server_port = 443;
        config.port_secure = 443;
        httpd_config_t *cfg_httpd = &config.httpd;
        cfg_httpd->stack_size = 10240;
    #else
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.server_port = 80;
        httpd_config_t *cfg_httpd = &config;
        cfg_httpd->stack_size = 4096;
    #endif

    cfg_httpd->max_open_sockets = 4;
    cfg_httpd->recv_wait_timeout = 8;
    cfg_httpd->send_wait_timeout = 8;
    cfg_httpd->lru_purge_enable = true;
    cfg_httpd->max_resp_headers = 16;
    cfg_httpd->max_uri_handlers = 32;

    eif_t temp_config = {
        .wifi_driver_config = WIFI_INIT_CONFIG_DEFAULT(),
        .wifi_power_mode = WIFI_PS_NONE,
        .wifi_attempt_delay_ms = 2000,
        .wifi_profiles_count = 3,
        .wifi_result_tests = {},

        .uri_handlers = NULL,
        .uri_handlers_count = 0,
        .server_config = config,

        #if defined(CONFIG_EIF_ENABLE_MDNS) || defined(CONFIG_EIF_ENABLE_TLS)
            .mdns_hostname = "",
        #endif
        #ifdef CONFIG_EIF_ENABLE_MDNS
            .mdns_instance_name = "",
            .mdns_txt_records = {},
            .mdns_txt_records_count = 0,
        #endif

        .user_pre_reboot_cb = NULL,

        .wifi_profile_index = 0,
        .wifi_handler_stop = false
    };

    cfg = temp_config;
    CORE_LOG(I, "Configuration initialized successfully");

    int result = xTaskCreate(memory_monitor_task, "memory_monitor_task", 2048, NULL, 5, NULL);
    if (result != pdPASS) {
        CORE_LOG(E, "FAILED to launch memory monitor! Error: %d", result);
        esp_restart(); 
    } else {
        CORE_LOG(I, "Memory monitor launched successfully");
    }
}

/* Public setters */

esp_err_t eif_register_pre_reboot_callback(eif_pre_reboot_callback_t cb) {
    CORE_LOG(D, _MSG_CALL_SETTER, __func__);

    if (cb == NULL) return ESP_ERR_INVALID_ARG;
    cfg.user_pre_reboot_cb = cb;
    return ESP_OK;
}

esp_err_t eif_set_wifi_config(
    wifi_init_config_t wifi_driver_config,
    wifi_ps_type_t wifi_power_mode,
    uint32_t wifi_attempt_delay_ms
) {
    CORE_LOG(D, _MSG_CALL_SETTER, __func__);

    cfg.wifi_driver_config = wifi_driver_config;
    cfg.wifi_power_mode = wifi_power_mode;
    cfg.wifi_attempt_delay_ms = wifi_attempt_delay_ms;

    return ESP_OK;
}

esp_err_t eif_set_wifi_profiles_count(uint8_t wifi_profiles_count) {
    CORE_LOG(D, _MSG_CALL_SETTER, __func__);

    if (wifi_profiles_count > WIFI_PROFILES_MAX_COUNT) {
        CORE_LOG(E, "wifi_profiles_count (%d) exceeds MAX (%d)", 
            wifi_profiles_count, WIFI_PROFILES_MAX_COUNT);
        return ESP_ERR_INVALID_ARG;
    }

    memset(cfg.wifi_result_tests, 0, sizeof(cfg.wifi_result_tests));
    cfg.wifi_profiles_count = wifi_profiles_count;

    return ESP_OK;
}

esp_err_t eif_set_uri_handlers(
    httpd_uri_t *uri_handlers, size_t uri_handlers_count
) {
    CORE_LOG(D, _MSG_CALL_SETTER, __func__);

    esp_err_t ret = ESP_OK;
    CHECK_NOT_NULL(uri_handlers, ESP_ERR_INVALID_ARG, goto cleanup);

    if (cfg.uri_handlers) free(cfg.uri_handlers);
    if (uri_handlers_count == 0) {
        cfg.uri_handlers = NULL;
        cfg.uri_handlers_count = 0;
        CORE_LOG(W, "No handlers to copy (count = 0)");
        goto cleanup;
    }

    cfg.uri_handlers = malloc(uri_handlers_count * sizeof(httpd_uri_t));
    CHECK_NOT_NULL(cfg.uri_handlers, ESP_ERR_NO_MEM, goto cleanup);

    memcpy(cfg.uri_handlers, uri_handlers, uri_handlers_count * sizeof(httpd_uri_t));
    cfg.uri_handlers_count = uri_handlers_count;
cleanup:
    if (ret != ESP_OK && cfg.uri_handlers) {
        free(cfg.uri_handlers);
        cfg.uri_handlers = NULL;
    }
    return ret;
}

#ifdef CONFIG_EIF_ENABLE_TLS
    esp_err_t eif_set_server_config_https(httpd_ssl_config_t *server_config) {
        CORE_LOG(D, _MSG_CALL_SETTER, __func__);

        esp_err_t ret = ESP_OK;
        CHECK_NOT_NULL(server_config, ESP_ERR_INVALID_ARG, goto cleanup);

        memcpy(&cfg.server_config, server_config, sizeof(httpd_ssl_config_t));

        if (cfg.server_config.cacert_pem) free((void *)cfg.server_config.cacert_pem);
        cfg.server_config.cacert_pem = NULL;
        cfg.server_config.cacert_len = 0;

        if (cfg.server_config.prvtkey_pem) free((void *)cfg.server_config.prvtkey_pem);
        cfg.server_config.prvtkey_pem = NULL;
        cfg.server_config.prvtkey_len = 0;
    cleanup:
        return ret;
    }
#else
    esp_err_t eif_set_server_config_http(httpd_config_t *server_config) {
        CORE_LOG(D, _MSG_CALL_SETTER, __func__);

        esp_err_t ret = ESP_OK;
        CHECK_NOT_NULL(server_config, ESP_ERR_INVALID_ARG, goto cleanup);

        memcpy(&cfg.server_config, server_config, sizeof(httpd_config_t));
    cleanup:
        return ret;
    }
#endif

#ifdef CONFIG_EIF_ENABLE_MDNS
    #include "mdns.h"

    esp_err_t eif_set_mdns(
        const char* mdns_hostname, const char* mdns_instance_name
    ) {
        CORE_LOG(D, _MSG_CALL_SETTER, __func__);

        esp_err_t ret = ESP_OK;
        CHECK_NOT_NULL(mdns_hostname, ESP_ERR_INVALID_ARG, goto cleanup);
        CHECK_NOT_NULL(mdns_instance_name, ESP_ERR_INVALID_ARG, goto cleanup);

        if (strlen(mdns_hostname) >= MDNS_HOSTNAME_PREFIX_MAX_LEN) {
            CORE_LOG(E, _ERR_INVALID_LEN, "mdns_hostname",
                strlen(mdns_hostname), 0, MDNS_HOSTNAME_PREFIX_MAX_LEN);
            ret = ESP_ERR_INVALID_ARG;
            goto cleanup; 
        }
        if (strlen(mdns_instance_name) >= MDNS_INSTANCE_NAME_MAX_LEN) {
            CORE_LOG(E, _ERR_INVALID_LEN, "mdns_instance_name",
                strlen(mdns_hostname), 0, MDNS_INSTANCE_NAME_MAX_LEN);
            ret = ESP_ERR_INVALID_ARG;
            goto cleanup; 
        }

        snprintf(cfg.mdns_hostname, 
            sizeof(cfg.mdns_hostname), "%s", mdns_hostname);
        snprintf(cfg.mdns_instance_name, 
            sizeof(cfg.mdns_instance_name), "%s", mdns_instance_name);
    cleanup:
        return ret;
    }

    esp_err_t eif_set_mdns_records(
        const mdns_txt_item_t txt_records[MDNS_TXT_RECORDS_MAX_COUNT], 
        size_t txt_records_count
    ) {
        CORE_LOG(D, _MSG_CALL_SETTER, __func__);

        esp_err_t ret = ESP_OK;
        CHECK_NOT_NULL(txt_records, ESP_ERR_INVALID_ARG, goto cleanup);

        if (txt_records_count == 0 && txt_records == NULL) {
            CORE_LOG(W, "Zero items count provided, clearing MDNS TXT records");
            cfg.mdns_txt_records[0].key = NULL;
            cfg.mdns_txt_records[0].value = NULL;
            cfg.mdns_txt_records_count = 0;
            return ESP_OK;
        }

        if (txt_records_count > MDNS_TXT_RECORDS_MAX_COUNT) {
            CORE_LOG(E, _ERR_INVALID_LEN, "txt_records_count",
                txt_records_count, 0, MDNS_TXT_RECORDS_MAX_COUNT);
            txt_records_count = MDNS_TXT_RECORDS_MAX_COUNT;
        }

        for (size_t i = 0; i < txt_records_count; i++) {
            if (txt_records[i].key == NULL || txt_records[i].value == NULL) {
                CORE_LOG(E, "Invalid MDNS record at index %zu (NULL detected)", i);
                return ESP_ERR_INVALID_ARG;
            }
            cfg.mdns_txt_records[i] = txt_records[i];
        }

        memset(
            &cfg.mdns_txt_records[txt_records_count], 0,
            (MDNS_TXT_RECORDS_MAX_COUNT - txt_records_count) * sizeof(mdns_txt_item_t)
        );

        CORE_LOG(I, "Successfully set %d 'mDNS TXT records'", txt_records_count);
    cleanup:
        return ret;
    }
#endif

/* Private setters */

#if defined(CONFIG_EIF_ENABLE_MDNS) || defined(CONFIG_EIF_ENABLE_TLS)
    void eif_format_mdns_hostname() {
        CORE_LOG(D, _MSG_CALL_UPDATER, __func__);

        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);

        const char* p = strlen(cfg.mdns_hostname) > 0 ? cfg.mdns_hostname : "device";

        char final_name[MDNS_HOSTNAME_FULL_MAX_LEN + 8]; 
        snprintf(final_name, sizeof(final_name), 
            "%s-%02x%02x%02x", p, mac[3], mac[4], mac[5]);

        strncpy(cfg.mdns_hostname, final_name, MDNS_HOSTNAME_FULL_MAX_LEN);
        cfg.mdns_hostname[MDNS_HOSTNAME_FULL_MAX_LEN - 1] = '\0';

        CORE_LOG(I, "Full mDNS hostname: %s", cfg.mdns_hostname);
    }
#endif

void eif_uri_handlers_count_update(void) {
    CORE_LOG(D, _MSG_CALL_UPDATER, __func__);

    size_t count = cfg.uri_handlers_count + DEFAULT_HANDLERS_COUNT;
    #ifdef CONFIG_EIF_ENABLE_TLS
        cfg.server_config.httpd.max_uri_handlers = count;
    #else
        cfg.server_config.max_uri_handlers = count;
    #endif
}

esp_err_t eif_wifi_p_index_set(uint8_t index) {
    CORE_LOG(D, _MSG_CALL_SETTER, __func__);
    if (index > cfg.wifi_profiles_count) return ESP_ERR_INVALID_ARG;
    CORE_LOG(D, _MSG_REPLACED, "wifi_profile_index", cfg.wifi_profile_index, index);
    cfg.wifi_profile_index = index;
    return ESP_OK;
}

esp_err_t eif_wifi_result_test_set(uint8_t index, wifi_test_result result) {
    CORE_LOG(D, _MSG_CALL_SETTER, __func__);
    if (index > cfg.wifi_profiles_count) return ESP_ERR_INVALID_ARG;
    CORE_LOG(I, 
        "`wifi_result_tests[%d]` replaced by .result = %s, .rssi = %d",
        index, result.result ? "true" : "false", result.rssi);
    cfg.wifi_result_tests[index] = result;

    return ESP_OK;
}

void eif_wifi_handler_stop_set(bool stop) {
    CORE_LOG(D, _MSG_CALL_SETTER, __func__);
    CORE_LOG(D, _MSG_REPLACED, "wifi_handler_stop", cfg.wifi_handler_stop, stop);
    cfg.wifi_handler_stop = stop;
}

#ifdef CONFIG_EIF_ENABLE_TLS
    esp_err_t eif_set_tls_creds_from_nvs(void) {
        CORE_LOG(D, _MSG_CALL_SETTER, __func__);

        esp_err_t ret = ESP_OK;
        char *c_buf = NULL, *k_buf = NULL;
        size_t c_len = 0, k_len = 0;
        uint8_t *c_copy = NULL, *k_copy = NULL;

        CHECK_ESP_ERR_T(E, nvs_tls_creds_load(&c_buf, &c_len, &k_buf, &k_len), 
            {}, GOTO_CLEANUP_ERR(), "");

        CORE_LOG(I, "'TLS credentials' loaded successfully");
        CORE_LOG(I, "Cert: %zu, Key: %zu", c_len, k_len);

        c_copy = malloc(c_len);
        k_copy = malloc(k_len);

        CHECK_NOT_NULL(c_copy, ESP_ERR_NO_MEM, goto cleanup);
        CHECK_NOT_NULL(k_copy, ESP_ERR_NO_MEM, goto cleanup);

        memcpy(c_copy, c_buf, c_len);
        memcpy(k_copy, k_buf, k_len);

        if (cfg.server_config.cacert_pem) free((void *)cfg.server_config.cacert_pem);
        if (cfg.server_config.prvtkey_pem) free((void *)cfg.server_config.prvtkey_pem);

        cfg.server_config.cacert_pem = c_copy;
        cfg.server_config.cacert_len = c_len;
        cfg.server_config.prvtkey_pem = k_copy;
        cfg.server_config.prvtkey_len = k_len;

        CORE_LOG(I, "TLS credentials successfully applied to config");
    cleanup:
        if (ret != ESP_OK) {
            if (c_copy) free(c_copy);
            if (k_copy) free(k_copy);
        }

        if (c_buf) free(c_buf);
        if (k_buf) free(k_buf);

        return ret;
    }
#endif

/* Private getters */

const eif_t* eif_get(void) {
    return &cfg;
}
