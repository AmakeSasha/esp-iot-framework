/* SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Library: esp_iot_framework_core
 * Folder: ./components/esp_iot_framework_core/src
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

#include <stdlib.h>
#include <esp_mac.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_ota_ops.h>
#ifdef CONFIG_EIF_ENABLE_MDNS
    #include <mdns.h>
#endif

#include "esp_iot_framework_core_macros.h"
#include "core_internal.h"

#define TAG "Core config"

/* --- */

#define CFG_ERR_ALLOCATE    "Failed to allocate %d bytes for '%s'"
#define CFG_ERR_INVALID_LEN "Invalid '%s' length: %u (allowed range: %u-%u)"

#if CONFIG_EIF_LOG_LEVEL >= EIF_LOG_LEVEL_D
    #define CFG_MSG_CALL_SETTER  "Calling the setter `%s`"
    #define CFG_MSG_CALL_UPDATER "Calling the setter `%s`"
    #define CFG_MSG_REPLACED     "`%s` with %d replaced by %d"
#endif

/* --- */

static eif_core_t cfg = {0};

/* ===================== */
/*    Framework Entry    */
/* ===================== */
esp_err_t eif_core_initialize(void) {
    esp_err_t ret = ESP_OK;

    EIF_LOG_D(MSG_CALL_SETTER, __func__);

    #ifdef CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG
        eif_core_log_init();
    #endif
    
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (running != NULL) {
        esp_ota_img_states_t state = ESP_OTA_IMG_UNDEFINED;
        if (esp_ota_get_state_partition(running, &state) == ESP_OK) {
            EIF_LOG_I("Partition state: %d", (int)state);
        } else {
            EIF_LOG_I("Partition state: unknown");
        }
    }

    /* --- INIT --- */
    eif_core_t temp_config = {0};

    wifi_init_config_t default_wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();

    temp_config.wifi_driver_config = default_wifi_cfg;
    temp_config.wifi_power_mode = WIFI_PS_NONE;
    temp_config.wifi_attempt_delay_ms = 2000U;
    temp_config.wifi_profiles_count = 2U;
    /* --- END INIT --- */

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
            (void)memcpy(cfg.mdns_hostname, hostname, hostname_len);
            cfg.mdns_hostname[hostname_len] = '\0';

            (void)memcpy(cfg.mdns_instance_name, instance_name, instance_len);
            cfg.mdns_instance_name[instance_len] = '\0';
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
    esp_err_t eif_format_mdns_hostname(void) {
        esp_err_t ret = ESP_OK;

        uint8_t mac[6] = {0};
        const char * hostname = cfg.mdns_hostname;
        const char hex_chars[] = "0123456789abcdef";
        char final_name[MDNS_HOSTNAME_FULL_MAX_LEN] = {0};

        EIF_LOG_D(CFG_MSG_CALL_UPDATER, __func__);

        ret = esp_read_mac(mac, ESP_MAC_WIFI_STA);
        if (ret == ESP_OK) {
            if (eif_strempty(cfg.mdns_hostname)) {
                hostname = "device";
            }

            size_t base_len = eif_strnlen(hostname, MDNS_HOSTNAME_FULL_MAX_LEN);
            size_t total_len = base_len + 7U;

            if (total_len >= (size_t)MDNS_HOSTNAME_FULL_MAX_LEN) {
                ret = ESP_ERR_INVALID_SIZE;
            } else {
                (void)memcpy(final_name, hostname, base_len);

                size_t pos = base_len;
                final_name[pos] = '-';
                pos++;

                for (size_t i = 3U; i < 6U; i++) {
                    size_t hi = ((size_t)mac[i] >> 4U) & 0x0FU;
                    size_t lo = (size_t)mac[i] & 0x0FU;

                    final_name[pos] = hex_chars[hi];
                    pos++;
                    final_name[pos] = hex_chars[lo];
                    pos++;
                }
                final_name[pos] = '\0';
            }
        }

        if (ret == ESP_OK) {
            (void)memcpy((void *)cfg.mdns_hostname, (const void *)final_name,
                (size_t)MDNS_HOSTNAME_FULL_MAX_LEN);
            cfg.mdns_hostname[MDNS_HOSTNAME_FULL_MAX_LEN - 1U] = '\0';

            if (eif_strempty(cfg.mdns_instance_name)) {
                (void)memcpy(
                    (void *)cfg.mdns_instance_name, 
                    (const void *)cfg.mdns_hostname,
                    sizeof(cfg.mdns_instance_name)
                );
                cfg.mdns_instance_name[sizeof(cfg.mdns_instance_name) - 1U]='\0';
            }

            EIF_LOG_I("Full mDNS hostname: %s", cfg.mdns_hostname);
            EIF_LOG_I("mDNS instance name: %s", cfg.mdns_instance_name);
        }

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