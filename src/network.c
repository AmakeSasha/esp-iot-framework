/*
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp_iot_framework
 * Folder: src
 * File: network.c
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
#include "esp_log.h"
#include "esp_wifi.h"
#include "sdkconfig.h"
#include "esp_system.h"

#include "macros.h"
#include "core_internal.h"
#include "esp_iot_framework.h"

#ifdef CONFIG_EIF_ENABLE_MDNS
    #define TAG "mDNS"
    #include "mdns.h"
    #ifdef CONFIG_EIF_ENABLE_TLS
        #define WEB_PROTOCOL "https"
    #else
        #define WEB_PROTOCOL "http"
    #endif

    static esp_err_t mdns_initialize(void) {
        esp_err_t ret = ESP_OK;
        const eif_t *cfg = eif_get();

        if (strlen(cfg->mdns_hostname) == 0) {
            CORE_LOG(I, "mDNS is disabled by config");
            return ESP_OK;
        }

        mdns_free();

        CHECK_ESP_ERR_T(E, mdns_init(), {}, GOTO_CLEANUP_ERR(), 
            "Failed to initialize mDNS");
        
        CHECK_ESP_ERR_T(E, mdns_hostname_set(cfg->mdns_hostname), 
            {}, GOTO_CLEANUP_ERR(), 
            "Could not set hostname to '%s'", cfg->mdns_hostname);
        
        const char* instance_name = (strlen(cfg->mdns_instance_name) > 0) 
            ? cfg->mdns_instance_name : cfg->mdns_hostname;
        CHECK_ESP_ERR_T(E, mdns_instance_name_set(instance_name), 
            {}, {}, "Failed to set instance name: %s", instance_name);

        bool has_txt = (cfg->mdns_txt_records_count > 0);
        CHECK_ESP_ERR_T(W, mdns_service_add(
            cfg->mdns_instance_name, 
            #ifdef CONFIG_EIF_ENABLE_TLS
                "_https", "_tcp", 443,
            #else
                "_http", "_tcp", 80,
            #endif
            has_txt ? (mdns_txt_item_t *)cfg->mdns_txt_records : NULL, 
            has_txt ? cfg->mdns_txt_records_count : 0
        ), {}, {}, "Could not register mDNS txt records");

        CORE_LOG(I, "mDNS started. Link: %s://%s.local",
            WEB_PROTOCOL, cfg->mdns_hostname);
        return ESP_OK;
    cleanup:
        mdns_free();
        return ret;
    }

    static esp_err_t mdns_deinitialize(void) {
        CORE_LOG(I, "Stopping mDNS...");

        mdns_service_remove_all();
        mdns_free();

        CORE_LOG(I, "mDNS deinitialized");
        return ESP_OK;
    }

    #undef TAG
#endif

/* --- */

#define TAG "Wifi Config"

esp_err_t wifi_set_config_from_profile(uint8_t index) {
    eif_wifi_p_index_set(index);

    char ssid[SSID_MAX_LEN + 1] = {0}, pass[PASSWORD_MAX_LEN + 1] = {0};
    CHECK_ESP_ERR_T(E, nvs_wifi_profile_load(index, ssid, pass), 
        {}, return err, "Failed to load profile #%u", index);

    if (strlen(ssid) == 0) {
        CORE_LOG(D, "Profile #%u has an empty SSID, skipping...", index);
        return ESP_ERR_INVALID_ARG;
    }

    wifi_config_t w_cfg = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_OPEN,
            .pmf_cfg = {.capable = true, .required = false}
        }
    };

    strncpy((char*)w_cfg.sta.ssid, ssid, sizeof(w_cfg.sta.ssid));
    w_cfg.sta.ssid[sizeof(w_cfg.sta.ssid) - 1] = '\0';

    strncpy((char*)w_cfg.sta.password, pass, sizeof(w_cfg.sta.password));
    w_cfg.sta.password[sizeof(w_cfg.sta.password) - 1] = '\0';


    CHECK_ESP_ERR_T(E, esp_wifi_set_config(WIFI_IF_STA, &w_cfg),
        if (err == ESP_ERR_WIFI_NOT_STARTED || err == ESP_ERR_WIFI_STOP_STATE) {
            CORE_LOG(W, "WiFi driver is not ready yet, profile #%u queued", 
                index);
        }, return err, "Driver rejected config for profile %u", index);

    CORE_LOG(I, "Active config switched to profile #%u. SSID: '%s'", 
        index, ssid);
    return ESP_OK;
}

#undef TAG

/* --- */

#define TAG "wifiTestTask"

void wifi_test_task(void *pvParameters) {
    uint32_t new_index = (uint32_t)(uintptr_t)pvParameters;
    wifi_test_result res = {false, -127};
    
    vTaskDelay(pdMS_TO_TICKS(500)); 

    const eif_t *cfg = eif_get();
    uint8_t origin_index = cfg->wifi_profile_index;

    CORE_LOG(I, "Starting WiFi test for param #%zu", new_index);

    eif_wifi_handler_stop_set(true);
    CHECK_ESP_ERR_T(E, esp_wifi_disconnect(), 
        if (err == ESP_ERR_WIFI_NOT_STARTED) break, {}, 
            "Disconnecting from current AP failed");

    vTaskDelay(pdMS_TO_TICKS(500));

    CHECK_ESP_ERR_T(E, wifi_set_config_from_profile(new_index), 
        {}, goto cleanup, 
        "Applying configuration for param #%zu failed", new_index);
    CHECK_ESP_ERR_T(E, esp_wifi_connect(), {}, goto cleanup, 
        "Initiating connection to param #%zu failed", new_index);

    for (int i = 0; i < 30; i++) {
        vTaskDelay(pdMS_TO_TICKS(500));
         
        esp_netif_ip_info_t ip_info;
        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        wifi_ap_record_t info; 
        
        esp_err_t ip_err = esp_netif_get_ip_info(netif, &ip_info);
        if (ip_err != ESP_OK) {
            CORE_LOG(W, "Failed to get IP info: %s", esp_err_to_name(ip_err));
            continue;
        }

        if (netif && ip_info.ip.addr != 0) {
            res.result = true;
            if (esp_wifi_sta_get_ap_info(&info) == ESP_OK) {
                res.rssi = info.rssi;
            }
            break;
        }

        CORE_LOG(D, "Waiting for IP address... (%d/30)", i + 1);
    }

cleanup:
    eif_wifi_result_test_set(new_index, res);
    CORE_LOG(I, "Test finished. Result: %s, RSSI: %d dBm", 
        res.result ? "SUCCESS" : "FAIL", res.rssi);

    CORE_LOG(I, "Restoring connection to param #%zu", origin_index);

    CHECK_ESP_ERR_T(E, esp_wifi_disconnect(), {}, {}, 
        "Disconnecting before restore failed");
    vTaskDelay(pdMS_TO_TICKS(300));
    CHECK_ESP_ERR_T(E, wifi_set_config_from_profile(origin_index), {}, {}, 
        "Restoring original config failed");
    CHECK_ESP_ERR_T(E, esp_wifi_connect(), {}, {}, 
        "Reconnecting to original AP failed");

    eif_wifi_handler_stop_set(false);

    CORE_LOG(I, "Task completed, self-deleting...");
    vTaskDelete(NULL);
}

#undef TAG

/* --- */

#define TAG "WiFi Handler"

static void wifi_event_handler(
    void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data
) {
    const eif_t *cfg = eif_get();
    if (cfg->wifi_handler_stop) return;

    switch (event_id) {
        case WIFI_EVENT_STA_START:  
            CORE_LOG(I, "WiFi STA started by profile #%u", 
                cfg->wifi_profile_index);
            CHECK_ESP_ERR_T(E, esp_wifi_connect(), {}, return, 
                "Initial connect failed");
            break;

        case WIFI_EVENT_STA_DISCONNECTED: {
            wifi_event_sta_disconnected_t* disconn = 
                (wifi_event_sta_disconnected_t*)event_data;
            CORE_LOG(I, "Disconnected, reason: %d", disconn->reason);

            bool is_wifi_set_config = false;
            while (1) {
                uint8_t next_index = (cfg->wifi_profile_index + 1)
                    % cfg->wifi_profiles_count;

                CHECK_ESP_ERR_T(W, 
                    wifi_set_config_from_profile(next_index), 
                    is_wifi_set_config = (err == ESP_OK), 
                    {}, "Failed to switch to profile #%u", next_index);
                
                if (is_wifi_set_config) break;
                vTaskDelay(pdMS_TO_TICKS(100));
            }

            vTaskDelay(pdMS_TO_TICKS(cfg->wifi_attempt_delay_ms));
            CHECK_ESP_ERR_T(E, esp_wifi_connect(), {}, return, 
                "Reconnect attempt failed");
            break;
        }

        case WIFI_EVENT_STA_CONNECTED:
            CORE_LOG(I, "Connected to WiFi by profile #%u",
                cfg->wifi_profile_index);
            break;

        default:
            CORE_LOG(W, "Unhandled WiFi event: %" PRId32, event_id);
            break;
    }
}

#undef TAG
#define TAG "IP Handler"

static void ip_event_handler(
    void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data
) {
    const eif_t *cfg = eif_get();
    if (cfg->wifi_handler_stop) return;

    switch (event_id) {
        case IP_EVENT_STA_GOT_IP: {
            ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
            CORE_LOG(I, "System got IP: " IPSTR, IP2STR(&event->ip_info.ip));

            #ifdef CONFIG_EIF_ENABLE_MDNS
                CHECK_ESP_ERR_T(E, mdns_initialize(), {}, {}, 
                    "mDNS startup failed");
                vTaskDelay(pdMS_TO_TICKS(100));
            #endif
            CHECK_ESP_ERR_T(E, eif_server_launch(), {}, {}, 
                "Web server launch failed");
            break;
        }
        case IP_EVENT_STA_LOST_IP:
            CORE_LOG(W, "IP address lost, taking services offline...");

            #ifdef CONFIG_EIF_ENABLE_MDNS
                mdns_deinitialize();
            #endif

            break;
    }
}

#undef TAG

/* --- */

#define TAG "WiFi Launch"

esp_err_t eif_wifi_initialize(void) {
    esp_err_t ret = ESP_OK;
    const eif_t *cfg = eif_get();

    if (cfg->wifi_profiles_count == 0) {
        CORE_LOG(E, "No WiFi networks found in configuration!");
        return ESP_ERR_INVALID_ARG;
    }

    CHECK_ESP_ERR_T(E, esp_netif_init(), {}, return ret, 
        "Network interface init failed");
    CHECK_ESP_ERR_T(E, esp_event_loop_create_default(), {}, return ret, 
        "Event loop creation failed");

    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    if (sta_netif == NULL) {
        CORE_LOG(E, "Failed to allocate memory for STA interface");
        return ESP_ERR_NO_MEM;
    }

    CHECK_ESP_ERR_T(E, esp_wifi_init(&cfg->wifi_driver_config), {}, 
        GOTO_CLEANUP_ERR(), "Driver init failed");
    CHECK_ESP_ERR_T(E, esp_wifi_set_mode(WIFI_MODE_STA), {}, 
        GOTO_CLEANUP_ERR(), "Mode selection failed");

    esp_event_handler_register(WIFI_EVENT, 
        ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, 
        IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL);

    CHECK_ESP_ERR_T(W, esp_wifi_set_ps(cfg->wifi_power_mode), 
        CORE_LOG(I, "Power save mode set to: %d", cfg->wifi_power_mode), 
        {}, "Could not set power mode %d", cfg->wifi_power_mode);

    CHECK_ESP_ERR_T(E, wifi_set_config_from_profile(0), {}, GOTO_CLEANUP_ERR(), 
        "Initial config application failed");
    CHECK_ESP_ERR_T(E, esp_wifi_start(), {}, GOTO_CLEANUP_ERR(), 
        "WiFi start failed");

    #if defined(CONFIG_EIF_ENABLE_MDNS) || defined(CONFIG_EIF_ENABLE_TLS)
        eif_format_mdns_hostname();
    #endif
    eif_uri_handlers_count_update();
    eif_wifi_handler_stop_set(false);
    CORE_LOG(I, "WiFi subsystem is up and running");
    return ESP_OK;

cleanup:
    esp_wifi_stop();
    esp_wifi_deinit();
    return ret;
}
