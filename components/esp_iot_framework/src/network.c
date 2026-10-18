/* SPDX-FileCopyrightText: 2026 AmakeSasha
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Path: ./components/esp_iot_framework/src/network.c
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

#include <mdns.h>
#include <string.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_system.h>

#include "private_include/macros.h"
#include "private_include/internal.h"

#ifdef CONFIG_EIF_ENABLE_TLS
    #define WEB_PROTOCOL  "https"
    #define MDNS_PROTOCOL "_eif_https"
    #define MDNS_PORT     443
#else
    #define WEB_PROTOCOL  "http"
    #define MDNS_PROTOCOL "_eif_http"
    #define MDNS_PORT     80
#endif

static uint8_t current_wifi_profile_index = 0U;

uint8_t wifi_get_current_profile_index(void) {
    return current_wifi_profile_index;
}



#define TAG "EIF mDNS"

static esp_err_t mdns_initialize(void) {
    esp_err_t ret = ESP_OK;
    const eif_config_t * const cfg = eif_config_get();

    /* @note Testing revealed that omitting this call triggers hard kernel panic
     *       (IllegalInstruction). Forcing an explicit cleanup here prevents the crash and ensures
     *       stability. This error is presumably caused by asynchronous cleanup of mDNS services. */
    (void)mdns_service_remove_all();

    EIF_TRY(mdns_init());
    EIF_TRY(mdns_hostname_set(cfg->mdns_hostname));
    EIF_TRY(mdns_instance_name_set(cfg->mdns_instance_name));
    EIF_TRY(mdns_service_add(
        cfg->mdns_instance_name, MDNS_PROTOCOL, "_tcp", MDNS_PORT,
        (mdns_txt_item_t *)cfg->mdns_txt_records,
        cfg->mdns_txt_records_count
    ));

    EIF_LOG_I("Started. Link: %s://%s.local", WEB_PROTOCOL, cfg->mdns_hostname);

cleanup:
    if (ret != ESP_OK) {
        EIF_LOG_E("Initialization failed, err: %s", esp_err_to_name(ret));
        (void)mdns_service_remove_all();
    }
    return ret;
}

#undef TAG



/* --- */



#define TAG "EIF Wi-Fi"

esp_err_t eif_wifi_set_config_from_profile() {
    esp_err_t ret = ESP_OK;
    char ssid[NVS_WIFI_SSID_MAX_LEN + 1U] = {0};
    char pass[NVS_WIFI_PASS_MAX_LEN + 1U] = {0};
    wifi_config_t w_cfg = {
        .sta.threshold.authmode = WIFI_AUTH_OPEN,
        .sta.pmf_cfg = {
            .capable = true,
            .required = false
        }
    };

    EIF_TRY(nvs_wifi_profile_load(current_wifi_profile_index, &ssid, &pass));

    if (ssid[0] == '\0') {
        ret = ESP_ERR_INVALID_STATE;
        goto cleanup;
    }

    (void)memcpy((void *)w_cfg.sta.ssid, (const void *)ssid, NVS_WIFI_SSID_MAX_LEN + 1U);
    (void)memcpy((void *)w_cfg.sta.password, (const void *)pass, NVS_WIFI_PASS_MAX_LEN + 1U);

    EIF_TRY(esp_wifi_set_config(WIFI_IF_STA, &w_cfg));

    EIF_LOG_I("Active config switched to profile #%u. SSID: '%s'", current_wifi_profile_index, ssid);

cleanup:
    if ((ret == ESP_ERR_WIFI_NOT_STARTED) || (ret == ESP_ERR_WIFI_STOP_STATE)) {
        EIF_LOG_W("Driver is not ready yet, profile #%u queued", current_wifi_profile_index);
        ret = ESP_OK;
    } else if (ret == ESP_ERR_INVALID_STATE) {
        EIF_LOG_D("Profile #%u has an empty SSID, skipping...", current_wifi_profile_index);
    } else if (ret != ESP_OK) {
        EIF_LOG_E("Failed to switch active config to profile #%u, err: %s",
                  current_wifi_profile_index, esp_err_to_name(ret));
    }
    return ret;
}

/* --- */



static void callback_on_wifi_sta_start(void* arg, esp_event_base_t base, int32_t id, void* data) {
    EIF_LOG_I("STA started. Loading first profile and connecting...");

    (void)eif_wifi_set_config_from_profile(0);
    (void)esp_wifi_connect();
}

static void callback_on_wifi_disconnect(void* arg, esp_event_base_t base, int32_t id, void* data) {
    bool profile_found = false;

    EIF_LOG_W("Disconnected (reason: %d). Reconnecting...",
              ((wifi_event_sta_disconnected_t*)data)->reason);

    for (uint8_t i = 0U; i < CONFIG_EIF_CUSTOM_WIFI_PROFILES_COUNT; i++) {
        current_wifi_profile_index++;
        if (current_wifi_profile_index >= CONFIG_EIF_CUSTOM_WIFI_PROFILES_COUNT) {
            current_wifi_profile_index = 0U;
        }

        EIF_LOG_W("Trying profile #%u...", current_wifi_profile_index);

        if (eif_wifi_set_config_from_profile() == ESP_OK) {
            profile_found = true;
            break;
        }
    }

    if (!profile_found) {
        EIF_LOG_W("No usable Wi-Fi profile found. Retrying current config...");
    }

    (void)esp_wifi_connect();
}

static void callback_on_wifi_connected(void* arg, esp_event_base_t base, int32_t id, void* data) {
    EIF_LOG_I("Connected successfully to AP: '%s' (channel: %d). Waiting for IP...",
              ((wifi_event_sta_connected_t*)data)->ssid, 
              ((wifi_event_sta_connected_t*)data)->channel);
}



static void callback_on_ip_got(void* arg, esp_event_base_t base, int32_t id, void* data) {
    static bool server_launched = false;

    EIF_LOG_I("Station got IP: " IPSTR ". Launching services...",
              IP2STR(&((ip_event_got_ip_t*)data)->ip_info.ip));
        
    if (!server_launched) {
        (void)server_launch();
        server_launched = true;
    }
    (void)mdns_initialize(); 
}

static void callback_on_ip_lost(void* arg, esp_event_base_t base, int32_t id, void* data) {
    EIF_LOG_W("Station lost IP address. Cleaning up network services...");

    (void)mdns_service_remove_all();
}



/* --- */

esp_err_t wifi_initialize(void) {
    esp_err_t ret = ESP_OK;
    esp_netif_t *sta_netif = {0};
    const eif_config_t * const cfg = eif_config_get();
    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();

    EIF_TRY(esp_netif_init());
    EIF_TRY(esp_event_loop_create_default());

    sta_netif = esp_netif_create_default_wifi_sta();
    EIF_CHECK_NOT_NULL(sta_netif, ESP_ERR_NO_MEM);

    EIF_TRY(esp_wifi_init(&wifi_cfg));
    EIF_TRY(esp_wifi_set_mode(WIFI_MODE_STA));
    EIF_TRY(esp_netif_set_hostname(sta_netif, cfg->mdns_hostname));

    EIF_TRY(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START,
                                       &callback_on_wifi_sta_start,  NULL));
    EIF_TRY(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
                                       &callback_on_wifi_disconnect, NULL));
    EIF_TRY(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED,
                                       &callback_on_wifi_connected,  NULL));

    EIF_TRY(esp_event_handler_register(IP_EVENT,   IP_EVENT_STA_GOT_IP,
                                       &callback_on_ip_got,          NULL));
    EIF_TRY(esp_event_handler_register(IP_EVENT,   IP_EVENT_STA_LOST_IP,
                                       &callback_on_ip_lost,         NULL));

    EIF_TRY(esp_wifi_start());
    
    if (esp_wifi_set_ps(cfg->wifi_power_mode) != ESP_OK) {
        EIF_LOG_W("Failed to set power save mode: %d", cfg->wifi_power_mode);
    } else {
        EIF_LOG_I("Power save mode set to: %d", cfg->wifi_power_mode);
    }
    
    EIF_LOG_I("Subsystem initialized");

cleanup:
    if (ret != ESP_OK) {
        EIF_LOG_E("Initialization failed, err: %s", esp_err_to_name(ret));
    }
    return ret;
}



/* --- */

bool wifi_is_ssid_available(const char *const ssid) {
    if ((ssid == NULL) || (ssid[0] == '\0')) {
        return false;
    }

    wifi_scan_config_t scan_config = {
        .ssid = (uint8_t *)ssid,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 100,
        .scan_time.active.max = 300
    };

    EIF_LOG_I("Scanning for SSID: '%s'", ssid);

    if (esp_wifi_scan_start(&scan_config, true) != ESP_OK) {
        return false;
    }

    uint16_t ap_count = 0;
    (void)esp_wifi_scan_get_ap_num(&ap_count);
    (void)esp_wifi_scan_stop();

    return (ap_count > 0U);
}