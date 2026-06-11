/* SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Library: esp_iot_framework_core
 * Folder: components/esp_iot_framework_core/src
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

#include "sdkconfig.h"

#include <string.h>
#include <stdlib.h>
#include "esp_mac.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_ota_ops.h"
#ifdef CONFIG_EIF_ENABLE_TLS
    #include "esp_https_server.h"
#else
    #include "esp_http_server.h"
#endif
#ifdef CONFIG_EIF_ENABLE_MDNS
    #include "mdns.h"
#endif

#include "esp_iot_framework_core_macros.h"
#include "core_internal.h"

#define TAG "Core config"

/* --- */

#define CFG_ERR_ALLOCATE    "Failed to allocate %d bytes for '%s'"
#define CFG_ERR_INVALID_LEN "Invalid '%s' length: %u (allowed range: %u-%u)"

#define CFG_MSG_CALL_SETTER  "Calling the setter `%s`"
#define CFG_MSG_CALL_UPDATER "Calling the setter `%s`"
#define CFG_MSG_REPLACED     "`%s` with %d replaced by %d"

/* --- */

static eif_core_t cfg = {0};

/* ===================== */
/*    Framework Entry    */
/* ===================== */

esp_err_t eif_core_initialize(void) {
    esp_err_t ret = ESP_OK;

    EIF_LOG_D(MSG_CALL_SETTER, __func__);

    const esp_partition_t *running = esp_ota_get_running_partition();
    if (running != NULL) {
        esp_ota_img_states_t state = ESP_OTA_IMG_UNDEFINED;
        if (esp_ota_get_state_partition(running, &state) == ESP_OK) {
            EIF_LOG_I("Partition state: %d", (int)state);
        } else {
            EIF_LOG_I("Partition state: unknown");
        }
    }

    eif_core_t temp_config = {
        .wifi_driver_config = WIFI_INIT_CONFIG_DEFAULT(),
        .wifi_power_mode = WIFI_PS_NONE,
        .wifi_attempt_delay_ms = 2000,
        .wifi_profiles_count = 3,
        .wifi_test_results = {{0}},

        #if defined(CONFIG_EIF_ENABLE_MDNS) || defined(CONFIG_EIF_ENABLE_TLS)
            .mdns_hostname = {0},
        #endif
        #ifdef CONFIG_EIF_ENABLE_MDNS
            .mdns_instance_name = {0},
            .mdns_txt_records = {{0}},
            .mdns_txt_records_count = 0,
        #endif

        .handler_ip_got = NULL,
        .handler_ip_lost = NULL,
        .handler_system_reboot = NULL,

        .current_wifi_profile_index = 0,
        .wifi_handler_stop = false
    };

    for (uint32_t i = 0U; i < EIF_WIFI_PROFILES_MAX_COUNT; i++) {
        temp_config.wifi_test_results[i].connected = false;
        temp_config.wifi_test_results[i].rssi = -127;
    }

    cfg = temp_config;

    EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_task_memory_monitor_launch(),
        "Failed to launch memory monitor");
    if (ret != ESP_OK) {
        /* @note Since this is the initial stage, no active services require 
         * a graceful shutdown; immediate restart is justified. */
        esp_restart(); 
    }

    if (ret == ESP_OK) { 
        EIF_LOG_I("Configuration initialized successfully");
    }

    /* Cleanup */
    return ret;
}

/* Public setters */

esp_err_t eif_register_handler_system_reboot(eif_handler_system_t handler) {
    esp_err_t ret = ESP_OK;

    EIF_LOG_D(CFG_MSG_CALL_SETTER, __func__);

    EIF_IF_OK_CHECK_NOT_NULL(ret, handler, ESP_ERR_INVALID_ARG);
    if (ret == ESP_OK) {
        cfg.handler_system_reboot = handler;
    }

    /* Cleanup */
    return ret;
}

esp_err_t eif_register_handler_ip_got(eif_handler_ip_t handler) {
    esp_err_t ret = ESP_OK;

    EIF_LOG_D(CFG_MSG_CALL_SETTER, __func__);

    EIF_IF_OK_CHECK_NOT_NULL(ret, handler, ESP_ERR_INVALID_ARG);
    if (ret == ESP_OK) {
        cfg.handler_ip_got = handler;
    }

    /* Cleanup */
    return ret;
}

esp_err_t eif_register_handler_ip_lost(eif_handler_ip_t handler) {
    esp_err_t ret = ESP_OK;

    EIF_LOG_D(CFG_MSG_CALL_SETTER, __func__);

    EIF_IF_OK_CHECK_NOT_NULL(ret, handler, ESP_ERR_INVALID_ARG);
    if (ret == ESP_OK) {
        cfg.handler_ip_lost = handler;
    }

    /* Cleanup */
    return ret;
}
/* end Framework Entry */



/* ========================= */
/*    Wi-Fi Configuration    */
/* ========================= */

uint8_t eif_wifi_get_profiles_count (void) {
    return cfg.wifi_profiles_count;
}

uint8_t eif_wifi_get_current_profile_index(void) {
    return cfg.current_wifi_profile_index;

}

esp_err_t eif_wifi_get_test_result(
    uint8_t index, eif_wifi_test_result * const out_result 
) {
    esp_err_t ret = ESP_OK;

    EIF_LOG_D(CFG_MSG_CALL_GETTER, __func__);

    if (index > cfg.wifi_profiles_count) {
        EIF_LOG_E("Index (%d) is too long (should be up to %d)",
            index, cfg.wifi_profiles_count);
        ret = ESP_ERR_INVALID_ARG;
    }
    if (ret == ESP_OK) {
        (void)memcpy(out_result, &cfg.wifi_test_results[index], 
            sizeof(eif_wifi_test_result));
    }

    return ret;
}

esp_err_t eif_set_wifi_config(
    const wifi_init_config_t *wifi_driver_config,
    wifi_ps_type_t wifi_power_mode,
    uint32_t wifi_attempt_delay_ms
) {
    esp_err_t ret = ESP_OK;

    EIF_LOG_D(CFG_MSG_CALL_SETTER, __func__);

    EIF_IF_OK_CHECK_NOT_NULL(ret, wifi_driver_config, ESP_ERR_INVALID_ARG);
    if (ret == ESP_OK) {
        cfg.wifi_driver_config = *wifi_driver_config;
        cfg.wifi_power_mode = wifi_power_mode;
        cfg.wifi_attempt_delay_ms = wifi_attempt_delay_ms;
    }

    /* Cleanup */
    return ret;
}

esp_err_t eif_set_wifi_profiles_count(uint8_t wifi_profiles_count) {
    esp_err_t ret = ESP_OK;

    EIF_LOG_D(CFG_MSG_CALL_SETTER, __func__);

    if (wifi_profiles_count > EIF_WIFI_PROFILES_MAX_COUNT) {
        EIF_LOG_E("wifi_profiles_count (%d) exceeds MAX (%d)", 
            wifi_profiles_count, EIF_WIFI_PROFILES_MAX_COUNT);
        ret = ESP_ERR_INVALID_ARG;
    }
    if (ret == ESP_OK) {
        (void)memset(cfg.wifi_test_results, 0, sizeof(cfg.wifi_test_results));

        for (uint32_t i = 0U; i < EIF_WIFI_PROFILES_MAX_COUNT; i++) {
            cfg.wifi_test_results[i].connected = false;
            cfg.wifi_test_results[i].rssi = -127;
        }

        cfg.wifi_profiles_count = wifi_profiles_count;
    }

    /* Cleanup */
    return ret;
}
/* end Wi-Fi Configuration */

/* ======================== */
/*    mDNS Configuration    */
/* ======================== */
#ifdef CONFIG_EIF_ENABLE_MDNS
    /* @deviation [Rule 21.6] The use of 'snprintf' is justified as the format 
     * string is constant and the input 'index' is a bounded 'uint8_t' value. 
     * Buffer safety is guaranteed by passing 'WIFI_KEY_LEN' as the size limit 
     * and explicitly checking the return value against the buffer size to 
     * ensure the output is not truncated and a null-terminator is present. 
     * This approach is more maintainable and less error-prone than manual 
     * string manipulation. */
    esp_err_t eif_set_mdns(
        const char * const hostname, const char * const instance_name
    ) {
        esp_err_t ret = ESP_OK;

        EIF_LOG_D(CFG_MSG_CALL_SETTER, __func__);

        EIF_IF_OK_CHECK_NOT_NULL(ret, hostname, ESP_ERR_INVALID_ARG);
        EIF_IF_OK_CHECK_NOT_NULL(ret, instance_name, ESP_ERR_INVALID_ARG);

        size_t hostname_len = eif_strnlen(hostname, MDNS_HOSTNAME_PREFIX_MAX_LEN);
        if (hostname_len >= (size_t)MDNS_HOSTNAME_PREFIX_MAX_LEN) {
            EIF_LOG_E(CFG_ERR_INVALID_LEN, "hostname",
                hostname_len, 0, MDNS_HOSTNAME_PREFIX_MAX_LEN);
            ret = ESP_ERR_INVALID_SIZE;
        }

        size_t instance_len = eif_strnlen(instance_name, MDNS_INSTANCE_NAME_MAX_LEN);
        if (instance_len >= (size_t)MDNS_INSTANCE_NAME_MAX_LEN) {
            EIF_LOG_E(CFG_ERR_INVALID_LEN, "instance_name",
                instance_len, 0, MDNS_INSTANCE_NAME_MAX_LEN);
            ret = ESP_ERR_INVALID_SIZE;
        }

        if (ret == ESP_OK) {
            /* Allowed by the '@deviation [Rule 21.6]' definition specified
             * before this function. */
            int res = snprintf(cfg.mdns_hostname, 
                sizeof(cfg.mdns_hostname), "%s", hostname);
            if ((res < 0) || (res >= (int)sizeof(cfg.mdns_hostname))) {
                ret = ESP_ERR_INVALID_SIZE;
            }
        }
        if (ret == ESP_OK) {
            /* Allowed by the '@deviation [Rule 21.6]' definition specified
             * before this function. */
            int res = snprintf(cfg.mdns_instance_name, 
                sizeof(cfg.mdns_instance_name), "%s", instance_name);
            if ((res < 0) || (res >= (int)sizeof(cfg.mdns_instance_name))) {
                ret = ESP_ERR_INVALID_SIZE;
            }
        }

        /* Cleanup */
        return ret;
    }

    esp_err_t eif_set_mdns_records(
        const mdns_txt_item_t txt_records[EIF_MDNS_TXT_RECORDS_MAX_COUNT], 
        size_t txt_records_count
    ) {
        esp_err_t ret = ESP_OK;

        EIF_LOG_D(CFG_MSG_CALL_SETTER, __func__);

        cfg.mdns_txt_records_count = 0U;
        (void)memset((void *)cfg.mdns_txt_records, 0U, 
            sizeof(cfg.mdns_txt_records));

        if (txt_records_count > (size_t)EIF_MDNS_TXT_RECORDS_MAX_COUNT) {
            EIF_LOG_E(CFG_ERR_INVALID_LEN, "txt_records_count",
                txt_records_count, 0, EIF_MDNS_TXT_RECORDS_MAX_COUNT);
            ret = ESP_ERR_INVALID_ARG;
        }
        if (txt_records_count > 0U) {
            EIF_IF_OK_CHECK_NOT_NULL(ret, txt_records, ESP_ERR_INVALID_ARG);
        }

        for (size_t i = 0; i < txt_records_count; i++) {
            EIF_IF_OK_CHECK_NOT_NULL(ret, txt_records[i].key, ESP_ERR_INVALID_SIZE);
            EIF_IF_OK_CHECK_NOT_NULL(ret, txt_records[i].value, ESP_ERR_INVALID_SIZE);

            if (ret == ESP_OK) {
                cfg.mdns_txt_records[i] = txt_records[i];
            } else {
                EIF_LOG_E("Invalid MDNS record at index %zu", i);
            }
        }
        if (ret == ESP_OK) {
            EIF_LOG_I("Successfully set %d 'mDNS TXT records'", txt_records_count);
        }

        /* Cleanup */
        return ret;
    }
#endif
/* end mDNS Configuration */

/* Private setters */
#if defined(CONFIG_EIF_ENABLE_MDNS) || defined(CONFIG_EIF_ENABLE_TLS)
    /* @deviation [Rule 21.6] The use of 'snprintf' is justified as the format 
     * string is constant and the input 'index' is a bounded 'uint8_t' value. 
     * Buffer safety is guaranteed by passing 'WIFI_KEY_LEN' as the size limit 
     * and explicitly checking the return value against the buffer size to 
     * ensure the output is not truncated and a null-terminator is present. 
     * This approach is more maintainable and less error-prone than manual 
     * string manipulation. */
    esp_err_t eif_format_mdns_hostname() {
        esp_err_t ret = ESP_OK;
        uint8_t mac[6] = {0};
        char final_name[MDNS_HOSTNAME_FULL_MAX_LEN] = {0};
        const char * hostname = cfg.mdns_hostname;

        EIF_LOG_D(CFG_MSG_CALL_UPDATER, __func__);

        ret = esp_read_mac(mac, ESP_MAC_WIFI_STA);
        if (ret == ESP_OK) {
            if (eif_strempty(cfg.mdns_hostname)) {
                hostname = "device";
            }

            /* Allowed by the '@deviation [Rule 21.6]' definition specified
             * before this function. */
            int res = snprintf(final_name, sizeof(final_name), 
                "%s-%02x%02x%02x", hostname, mac[3], mac[4], mac[5]);
            if ((res < 0) || (res >= (int)sizeof(final_name))) {
                ret = ESP_ERR_INVALID_SIZE;
            }
        }

        if (ret == ESP_OK) {
            (void)memcpy((void *)cfg.mdns_hostname, (const void *)final_name, 
                (size_t)MDNS_HOSTNAME_FULL_MAX_LEN);
            cfg.mdns_hostname[MDNS_HOSTNAME_FULL_MAX_LEN - 1U] = '\0';
        }
        if (ret == ESP_OK) {            
            if (eif_strempty(cfg.mdns_instance_name)) {
                (void)strlcpy(
                    cfg.mdns_instance_name, 
                    cfg.mdns_hostname,
                    sizeof(cfg.mdns_instance_name)
                );
            }

            EIF_LOG_I("Full mDNS hostname: %s", cfg.mdns_hostname);
            EIF_LOG_I("mDNS instance name: %s", cfg.mdns_instance_name);
        }

        /* Cleanup */
        return ret;
    }
#endif

esp_err_t eif_set_current_wifi_profile_index(uint8_t index) {
    esp_err_t ret = ESP_OK;

    EIF_LOG_D(CFG_MSG_CALL_SETTER, __func__);

    if (index > cfg.wifi_profiles_count) {
        ret = ESP_ERR_INVALID_ARG;
    }
    if (ret == ESP_OK) {
        EIF_LOG_D(CFG_MSG_REPLACED, "current_wifi_profile_index", 
            cfg.current_wifi_profile_index, index);
        cfg.current_wifi_profile_index = index;
    }

    /* Cleanup */
    return ret;
}

esp_err_t eif_set_wifi_result_test(uint8_t index, eif_wifi_test_result result) {
    esp_err_t ret = ESP_OK;

    EIF_LOG_D(_MSG_CALL_SETTER, __func__);

    if (index > cfg.wifi_profiles_count) {
        EIF_LOG_E("Index (%d) is too long (should be up to %d)",
            index, cfg.wifi_profiles_count);
        ret = ESP_ERR_INVALID_ARG;
    }
    if (ret == ESP_OK) {
        EIF_LOG_I(
            "`wifi_result_tests[%d]` replaced by .connected = %s, .rssi = %d",
            index, result.connected ? "true" : "false", result.rssi);
        cfg.wifi_test_results[index] = result;
    }

    return ret;
}

void eif_wifi_handler_stop_set(bool stop) {
    EIF_LOG_D(CFG_MSG_CALL_SETTER, __func__);
    EIF_LOG_D(CFG_MSG_REPLACED, "wifi_handler_stop", cfg.wifi_handler_stop, stop);
    cfg.wifi_handler_stop = stop;
}

/* Private getters */

const eif_core_t* eif_core_get(void) {
    EIF_LOG_D(CFG_MSG_CALL_GETTER, __func__);

    return &cfg;
}