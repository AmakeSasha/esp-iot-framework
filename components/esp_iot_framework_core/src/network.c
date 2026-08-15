/*
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Folder: ./components/esp_iot_framework_core/src
 * File: network.c
 * Library: esp_iot_framework_core
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
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_system.h>
#ifdef CONFIG_EIF_ENABLE_MDNS
    #include <mdns.h>
#endif

#include "esp_iot_framework_core_macros.h"
#include "core_internal.h"
#include "esp_iot_framework_core.h"

#ifdef CONFIG_EIF_ENABLE_MDNS
    #ifdef CONFIG_EIF_ENABLE_TLS
        #define WEB_PROTOCOL  "https"
        #define MDNS_PROTOCOL "_https"
        #define MDNS_PORT     443
    #else
        #define WEB_PROTOCOL  "http"
        #define MDNS_PROTOCOL "_http"
        #define MDNS_PORT     80
    #endif

    static esp_err_t mdns_initialize(void) {
        EIF_TAG_WITH_UNUSED "mDNS";

        esp_err_t ret = ESP_OK;
        const eif_core_t * const cfg = eif_core_get();

        /* @note Testing revealed that omitting this call triggers hard kernel
         * panic (IllegalInstruction). Forcing an explicit cleanup here prevents
         * the crash and ensures stability. This error is presumably caused by
         * asynchronous cleanup of mDNS services.*/
        (void)mdns_service_remove_all();

        EIF_IF_OK_CHECK_ESP_ERR_T(ret, mdns_init(), "Failed to initialize mDNS");
        EIF_IF_OK_CHECK_ESP_ERR_T(ret, mdns_hostname_set(cfg->mdns_hostname),
            "Could not set hostname to '%s'", cfg->mdns_hostname);

        EIF_IF_OK_CHECK_ESP_ERR_T(ret,
            mdns_instance_name_set(cfg->mdns_instance_name),
            "Failed to set instance name: %s", cfg->mdns_instance_name);

        /* @deviation [Rule 11.8] eif_core_get() intentionally returns a const pointer 
         * to protect core configuration data from accidental modification, enforcing 
         * changes strictly through dedicated setters. The explicit cast is required 
         * only because the third-party ESP-IDF 'mdns_service_add()' function misses 
         * 'const' in its signature for the TXT items argument. Making core data 
         * non-const would break the system data protection model. Code review of the 
         * ESP-IDF source confirms that 'mdns_service_add()' operates in read-only 
         * mode and does not modify the buffer, making this cast completely safe. */
        /* cppcheck-suppress misra-c2012-11.8 */
        EIF_IF_OK_CHECK_ESP_ERR_T(ret, mdns_service_add(
            cfg->mdns_instance_name, MDNS_PROTOCOL, "_tcp", MDNS_PORT,
            (mdns_txt_item_t *)cfg->mdns_txt_records,
            cfg->mdns_txt_records_count
        ), "Could not register mDNS txt records");

        if (ret == ESP_OK) {
            EIF_LOG_I("mDNS started. Link: %s://%s.local",
                WEB_PROTOCOL, cfg->mdns_hostname);
        }

        /* Cleanup */
        if (ret != ESP_OK) {
            mdns_free();
        }
        return ret;
    }

    static esp_err_t mdns_deinitialize(void) {
        EIF_TAG_WITH_UNUSED "mDNS";

        EIF_LOG_I("Stopping mDNS...");

        (void)mdns_service_remove_all();
        vTaskDelay(pdMS_TO_TICKS(100));
        mdns_free();

        EIF_LOG_I("mDNS deinitialized");
        return ESP_OK;
    }
#endif

/* --- */

esp_err_t eif_wifi_set_config_from_profile(uint8_t index) {
    EIF_TAG_WITH_UNUSED "Wifi Config";

    esp_err_t ret = ESP_OK;

    char ssid[EIF_WIFI_SSID_MAX_LEN] = {0};
    char pass[EIF_WIFI_PASS_MAX_LEN] = {0};
    /* @deviation [Rule 19.2] The use of 'union' is mandatory here as it is
     * part of the 'wifi_config_t' structure defined by the ESP-IDF SDK.
     * Manual zero-initialization via '{0}' used to ensure all union members
     * are in a safe, predictable state before accessing specific fields. */
    /* cppcheck-suppress misra-c2012-19.2 */
    wifi_config_t w_cfg;

    (void)memset(&w_cfg, 0, sizeof(w_cfg));

    w_cfg.sta.threshold.authmode = WIFI_AUTH_OPEN;
    w_cfg.sta.pmf_cfg.capable = true;
    w_cfg.sta.pmf_cfg.required = false;

    (void)eif_set_current_wifi_profile_index(index);

    EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_nvs_wifi_profile_load(index, ssid, pass),
        "Failed to load profile #%u", index);

    if ((ret == ESP_OK) && (eif_strempty(ssid))) {
        EIF_LOG_D("Profile #%u has an empty SSID, skipping...", index);
        ret = ESP_ERR_INVALID_ARG;
    }

    if (ret == ESP_OK) {
        size_t const s_len = eif_strnlen(ssid, sizeof(w_cfg.sta.ssid) - 1U);
        (void)memcpy((void *)w_cfg.sta.ssid, (const void *)ssid, s_len);
        w_cfg.sta.ssid[s_len] = '\0';
       
        size_t const p_len = eif_strnlen(pass, sizeof(w_cfg.sta.password) - 1U);
        (void)memcpy((void *)w_cfg.sta.password, (const void *)pass, p_len);
        w_cfg.sta.password[p_len] = '\0';
    }

    EIF_IF_OK_CHECK_ESP_ERR_T(ret, esp_wifi_set_config(WIFI_IF_STA, &w_cfg),
        "Driver rejected config for profile %u", index);

    if ((ret == ESP_ERR_WIFI_NOT_STARTED) || (ret == ESP_ERR_WIFI_STOP_STATE)) {
        EIF_LOG_W("Wi-Fi driver is not ready yet, profile #%u queued", index);
        ret = ESP_OK;
    }

    if (ret == ESP_OK) {
        EIF_LOG_I("Active config switched to profile #%u. SSID: '%s'",
            index, ssid);
    }

    /* Cleanup */
    return ret;
}

/* --- */

/* @deviation The parameter 'event_data' cannot be declared as 'const void*'
 * because the signature of this callback must strictly match the
 * 'esp_event_handler_t' type defined by the ESP-IDF SDK. Adding 'const' would
 * force an unsafe function pointer typecast during registration, risking severe
 * stack corruption if the SDK signature changes.
 * 
 * This approach is entirely safe as 'event_data' is treated as strictly
 * read-only within the handler body. No data write or modification is performed,
 * ensuring complete type safety and preventing any accidental corruption of
 * the OS event subsystem memory. */
static void wifi_event_handler(
    // cppcheck-suppress constParameterCallback
    void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data
) {
    EIF_TAG_WITH_UNUSED "Wi-Fi Handler";
    (void)arg;
    (void)event_base;

    esp_err_t ret = ESP_OK;
    const eif_core_t * const cfg = eif_core_get();

    if (!cfg->wifi_handler_stop) {
        switch (event_id) {
            case WIFI_EVENT_STA_START: 
                EIF_LOG_I("Wi-Fi STA started by profile #%u",
                    cfg->current_wifi_profile_index);
                EIF_IF_OK_CHECK_ESP_ERR_T(ret, esp_wifi_connect(),
                    "Initial connect failed");
                break;

            case WIFI_EVENT_STA_DISCONNECTED: {
                wifi_event_sta_disconnected_t disconn_data;
                (void)memcpy(&disconn_data, event_data, sizeof(disconn_data));

                EIF_LOG_I("Disconnected, reason: %d", disconn_data.reason);

                if (cfg->handler_ip_lost != NULL) {
                    EIF_IF_OK_CHECK_ESP_ERR_T(ret, cfg->handler_ip_lost(),
                        "Failed to execute 'IP_EVENT_STA_LOST_IP' handler");
                    vTaskDelay(pdMS_TO_TICKS(100));
                }

                bool is_wifi_set_config = false;
                while (!is_wifi_set_config) {
                    uint8_t next_index = (cfg->current_wifi_profile_index + 1U)
                        % cfg->wifi_profiles_count;

                    EIF_SHOW_ESP_ERR_T(ret,
                        eif_wifi_set_config_from_profile(next_index),
                        "Failed to switch to profile #%u", next_index);

                    is_wifi_set_config = (ret == ESP_OK);
                    vTaskDelay(pdMS_TO_TICKS(250));
                }

                vTaskDelay(pdMS_TO_TICKS(cfg->wifi_attempt_delay_ms));
                EIF_SHOW_ESP_ERR_T(ret, esp_wifi_connect(),
                    "Reconnect attempt failed");
                break;
            }

            case WIFI_EVENT_STA_CONNECTED:
                EIF_LOG_I("Connected to Wi-Fi by profile #%u",
                    cfg->current_wifi_profile_index);
                break;

            default:
                EIF_LOG_W("Unhandled Wi-Fi event: %" PRId32, event_id);
                break;
        }
    }

    /* Cleanup */
    (void)ret;
}

/* @deviation The parameter 'event_data' cannot be declared as 'const void*'
 * because the signature of this callback must strictly match the
 * 'esp_event_handler_t' type defined by the ESP-IDF SDK. Adding 'const' would
 * force an unsafe function pointer typecast during registration, risking severe
 * stack corruption if the SDK signature changes.
 * 
 * This approach is entirely safe as 'event_data' is treated as strictly
 * read-only within the handler body. No data write or modification is performed,
 * ensuring complete type safety and preventing any accidental corruption of
 * the OS event subsystem memory. */
static void ip_event_handler(
    // cppcheck-suppress constParameterCallback
    void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data
) {
    EIF_TAG_WITH_UNUSED "IP Handler";
    (void)arg;
    (void)event_base;

    esp_err_t ret = ESP_OK;
    const eif_core_t * const cfg = eif_core_get();

    if (!cfg->wifi_handler_stop) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP: {
                ip_event_got_ip_t got_ip;
                (void)memcpy(&got_ip, event_data, sizeof(got_ip));
                EIF_LOG_I("System got IP: " IPSTR, IP2STR(&got_ip.ip_info.ip));

                #ifdef CONFIG_EIF_ENABLE_MDNS
                    EIF_IF_OK_CHECK_ESP_ERR_T(ret, mdns_initialize(),
                        "mDNS startup failed");
                    vTaskDelay(pdMS_TO_TICKS(100));
                #endif
                if (cfg->handler_ip_got != NULL) {
                    EIF_IF_OK_CHECK_ESP_ERR_T(ret, cfg->handler_ip_got(),
                        "Failed to execute 'IP_EVENT_STA_GOT_IP' handler");
                }

                break;
            }
            case IP_EVENT_STA_LOST_IP:
                EIF_LOG_W("IP address lost, taking services offline...");

                #ifdef CONFIG_EIF_ENABLE_MDNS
                    (void)mdns_deinitialize();
                    vTaskDelay(pdMS_TO_TICKS(100));
                #endif
                if (cfg->handler_ip_lost != NULL) {
                    EIF_IF_OK_CHECK_ESP_ERR_T(ret, cfg->handler_ip_lost(),
                        "Failed to execute 'IP_EVENT_STA_LOST_IP' handler");
                }
                break;

            default:
                break;
        }
    }

    /* Cleanup */
    (void)ret;
}

/* --- */
esp_err_t eif_wifi_deinitialize(void) {
    EIF_TAG_WITH_UNUSED "Wi-Fi Deinitialize";

    esp_err_t ret = ESP_OK;
    const eif_core_t * const cfg = eif_core_get();

    eif_wifi_handler_stop_set(true);
    vTaskDelay(pdMS_TO_TICKS(100));
    if (cfg->handler_ip_lost != NULL) {
        EIF_SHOW_ESP_ERR_T(ret, cfg->handler_ip_lost(),
            "Failed to execute 'IP_EVENT_STA_LOST_IP' handler");
    }
    vTaskDelay(pdMS_TO_TICKS(100));

    #ifdef CONFIG_EIF_ENABLE_MDNS
        EIF_SHOW_ESP_ERR_T(ret, mdns_deinitialize(),
            "mDNS deinitialization failed");
        vTaskDelay(pdMS_TO_TICKS(100));
    #endif

    EIF_SHOW_ESP_ERR_T(ret, esp_event_handler_unregister(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler
    ), "Failed to unregister Wi-Fi event handler");
    EIF_SHOW_ESP_ERR_T(ret, esp_event_handler_unregister(
        IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler
    ), "Failed to unregister IP event handler");

    EIF_SHOW_ESP_ERR_T(ret, esp_wifi_disconnect(),
        "Failed to drop active connection");
    vTaskDelay(pdMS_TO_TICKS(100));
    EIF_SHOW_ESP_ERR_T(ret, esp_wifi_stop(),
        "Failed to stop Wi-Fi driver");
    vTaskDelay(pdMS_TO_TICKS(100));
    EIF_SHOW_ESP_ERR_T(ret, esp_wifi_deinit(),
        "Failed to deinitialize Wi-Fi stack");

    return ret;
}

esp_err_t eif_wifi_initialize(void) {
    EIF_TAG_WITH_UNUSED "Wi-Fi Initialize";

    esp_err_t ret = ESP_OK;
    esp_netif_t *sta_netif = {0};
    const eif_core_t * const cfg = eif_core_get();

    EIF_IF_OK_CHECK_ESP_ERR_T(ret, esp_netif_init(),
        "Network interface init failed");
    EIF_IF_OK_CHECK_ESP_ERR_T(ret, esp_event_loop_create_default(),
        "Event loop creation failed");

    if (ret == ESP_OK) {
        sta_netif = esp_netif_create_default_wifi_sta();
        if (sta_netif == NULL) {
            EIF_LOG_E("Failed to allocate memory for STA interface");
            ret = ESP_ERR_NO_MEM;
        }
    }
    (void)sta_netif;

    EIF_IF_OK_CHECK_ESP_ERR_T(ret, esp_wifi_init(&cfg->wifi_driver_config),
        "Driver init failed");
    EIF_IF_OK_CHECK_ESP_ERR_T(ret, esp_wifi_set_mode(WIFI_MODE_STA),
        "Mode selection failed");

    if (ret == ESP_OK) {
        EIF_IF_OK_CHECK_ESP_ERR_T(ret, esp_event_handler_register(
            WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL
        ), "WiFi event handler registration failed");
        EIF_IF_OK_CHECK_ESP_ERR_T(ret, esp_event_handler_register(
            IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL
        ), "IP event handler registration failed");
    }

    EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_wifi_set_config_from_profile(0),
        "Initial config application failed");
    EIF_IF_OK_CHECK_ESP_ERR_T(ret, esp_wifi_start(),
        "Wi-Fi start failed");

    if (ret == ESP_OK) {
        if (esp_wifi_set_ps(cfg->wifi_power_mode) != ESP_OK) {
            EIF_LOG_W("Could not set power mode %d", cfg->wifi_power_mode);
        } else {
            EIF_LOG_I("Power save mode set to: %d", cfg->wifi_power_mode);
        }
    }

    #if defined(CONFIG_EIF_ENABLE_MDNS) || defined(CONFIG_EIF_ENABLE_TLS)
        EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_format_mdns_hostname(),
            "Error formatting mDNS fields");

        if (ret == ESP_OK) {
            EIF_SHOW_ESP_ERR_T(ret,
                esp_netif_set_hostname(sta_netif, cfg->mdns_hostname),
                "Failed to set Wi-Fi DHCP hostname");

            ret = ESP_OK;
        }
    #endif

    if (ret == ESP_OK) {
        (void)eif_wifi_handler_stop_set(false);
        EIF_LOG_I("Wi-Fi subsystem is up and running");
    }

    /* Cleanup */
    if (ret != ESP_OK) {
        (void)eif_wifi_deinitialize();
    }
    return ret;
}