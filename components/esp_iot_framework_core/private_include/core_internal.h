/* SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Library: esp_iot_framework_core
 * Folder: ./components/esp_iot_framework_core/private_include
 * File: core_internal.h
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

#ifndef CORE_INTERNAL_H
#define CORE_INTERNAL_H

#include "sdkconfig.h"

#include <string.h>
#include <esp_wifi.h>
#ifdef CONFIG_EIF_ENABLE_MDNS
    #include <mdns.h>
#endif

#include "esp_iot_framework_core.h"
#include "esp_iot_framework_core_ext.h"
#include "esp_iot_framework_core_mdns.h"

/* config.c */

#if defined(CONFIG_EIF_ENABLE_MDNS) || defined(CONFIG_EIF_ENABLE_TLS)
    /* esp32-divece-aaccbb */
    #define MDNS_HOSTNAME_FULL_MAX_LEN 64U
    /* -aaccbb */
    #define MDNS_SUFFIX_MAC_LEN 7U
    /* esp32-divece */
    #define MDNS_HOSTNAME_PREFIX_MAX_LEN MDNS_HOSTNAME_FULL_MAX_LEN - MDNS_SUFFIX_MAC_LEN
#endif
#ifdef CONFIG_EIF_ENABLE_MDNS
    #define MDNS_INSTANCE_NAME_MAX_LEN 64U
#endif

/* ESP IoT Framework - eif */
typedef struct {
    /* eif_set_cfg_wifi */
    wifi_init_config_t wifi_driver_config;
    wifi_ps_type_t wifi_power_mode;
    uint32_t wifi_attempt_delay_ms;
    /* eif_set_wifi_profiles_count */
    size_t wifi_profiles_count;
    eif_wifi_test_result wifi_test_results[EIF_WIFI_PROFILES_MAX_COUNT];

    #if defined(CONFIG_EIF_ENABLE_MDNS) || defined(CONFIG_EIF_ENABLE_TLS)
        char mdns_hostname[MDNS_HOSTNAME_FULL_MAX_LEN];
    #endif
    #ifdef CONFIG_EIF_ENABLE_MDNS
        /* eif_set_mdns */
        char mdns_instance_name[MDNS_INSTANCE_NAME_MAX_LEN];
        /* eif_set_mdns_records */
        mdns_txt_item_t mdns_txt_records[EIF_MDNS_TXT_RECORDS_MAX_COUNT];
        size_t mdns_txt_records_count;
    #endif

    /* eif_register_event_handler_ip_got */
    eif_handler_ip_t handler_ip_got;
    /* eif_register_event_handler_ip_lost */
    eif_handler_ip_t handler_ip_lost;

    /* eif_register_handler_system_reboot */
    eif_handler_system_t handler_system_reboot;

    /* Core */
    uint8_t current_wifi_profile_index;
    bool wifi_handler_stop;
} eif_core_t;

const eif_core_t* eif_core_get(void);

esp_err_t eif_set_current_wifi_profile_index(uint8_t index);
esp_err_t eif_set_wifi_result_test(uint8_t index, eif_wifi_test_result result);
void eif_wifi_handler_stop_set(bool stop);

#if defined(CONFIG_EIF_ENABLE_MDNS) || defined(CONFIG_EIF_ENABLE_TLS)
    esp_err_t eif_format_mdns_hostname();
#endif

/* tls_maneger.c */
#ifdef CONFIG_EIF_ENABLE_TLS
    esp_err_t eif_tls_create_creds_and_nvs_save(void);
#endif

/* network.c */

esp_err_t eif_wifi_set_config_from_profile(uint8_t index);
esp_err_t eif_wifi_deinitialize(void);

/* system.c */

void eif_core_log_init(void);
esp_err_t eif_task_reboot_launch(void);
esp_err_t eif_task_memory_monitor_launch(void);
esp_err_t eif_task_wifi_test_launch(uint8_t profile_index);
#ifdef CONFIG_EIF_ENABLE_TLS
    esp_err_t eif_task_tls_recreate_launch(void);
#endif
esp_err_t eif_task_rollback_and_reboot_launch(void);

#endif