/*
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Folder: ./components/esp_iot_framework_core/include
 * File: esp_iot_framework_core.h
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

#ifndef ESP_IOT_FRAMEWORK_CORE_H
#define ESP_IOT_FRAMEWORK_CORE_H

#include <mdns.h>
#include <stdint.h>
#include <esp_err.h>
#include <stdbool.h>
#include <esp_wifi.h>

#ifdef __cplusplus
    extern "C" {
#endif

/* =============== Entry =============== */

/* Initializes the `CORE` of the framework. */
esp_err_t eif_core_initialize(void);
/* Initialize Non-Volatile Storage (NVS) and framework data. */
esp_err_t eif_nvs_initialize(void);



/* =============== Event Handlers =============== */

/* Prototype for system-level lifecycle event handlers. */
typedef esp_err_t (*eif_handler_system_t)(void);
/* Register a handler for the system reboot event. */
esp_err_t eif_register_handler_system_reboot(eif_handler_system_t handler);



/* =============== Constants =============== */

/* Maximum number of custom Wi-Fi profiles. */
#define EIF_WIFI_PROFILES_MAX_COUNT 9U
/* Default number of custom Wi-Fi profiles. */
#define EIF_WIFI_PROFILES_DEFAULT_COUNT 2U



/* =============== Wi-Fi Configuration =============== */

/* Configure Wi-Fi driver and power management policies. */
esp_err_t eif_set_wifi_config(
    const wifi_init_config_t *wifi_driver_config,
    wifi_ps_type_t wifi_power_mode,
    uint32_t wifi_attempt_delay_ms
);
/* Set the number of additional user Wi-Fi profiles. */
esp_err_t eif_set_wifi_profiles_count(uint8_t wifi_profiles_count);
/* Launch the Wi-Fi subsystem and automated network services. */
esp_err_t eif_wifi_initialize(void);



/* =============== mDNS Configuration =============== */

/* Maximum number of mDNS txt-records. */
#define EIF_MDNS_TXT_RECORDS_MAX_COUNT 32U
/* Maximum length of the mDNS hostname prefix including null-terminator (`\0`). */
#define EIF_MDNS_HOSTNAME_MAX_LEN      57U
/* Maximum length of the mDNS instance name (friendly name) including null-terminator (`\0`). */
#define EIF_MDNS_INSTANCE_NAME_MAX_LEN 64U

/* Configure the mDNS hostname and instance name. */
esp_err_t eif_set_mdns(
    const char * const hostname, const char * const instance_name
);

/* Set custom TXT records for mDNS service discovery. */
esp_err_t eif_set_mdns_records(
    const mdns_txt_item_t txt_records[EIF_MDNS_TXT_RECORDS_MAX_COUNT],
    size_t txt_records_count
);

#ifdef __cplusplus
    }
#endif
#endif