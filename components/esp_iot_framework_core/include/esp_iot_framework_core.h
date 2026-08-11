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

#include "sdkconfig.h"

#include <stdint.h>
#include <esp_err.h>
#include <stdbool.h>
#include <esp_wifi.h>

#ifdef __cplusplus
    extern "C" {
#endif

/**
 * @defgroup core_root Core
 * @copydoc md_docs_html_README_CORE
 * @{
 */
/**
 * @defgroup core_group Core
 * @brief The ecosystem engine. Provides framework initialization, Wi-Fi configuration, network profile management, and system lifecycle hooks.
 */
/**
 * @defgroup core_ext_group Core Extension
 * @brief Low-level API exposing NVS storage, FreeRTOS task management, IP event handlers, and internal configuration queries for creating nodes.
 */
/**
 * @defgroup core_macros_group Core Macros
 * @brief Preprocessor utilities for standardized logging, sequential error checking, and task spawning.
 */
/** @} */

/**
 * @addtogroup core_group Core
 * @{
 *
 * @details @note This group of modules is available when you include this line
 * at the beginning of the file.:
 * @code{c}
 * #include <esp_iot_framework_core.h>
 * @endcode
 *
 * The `CORE` serves as the essential foundation of the framework. It
 * handles the critical low-level tasks—memory orchestration and system
 * synchronization—to provide a reliable base for any application.
 *
 * This module is designed to be universal: whether you are building a simple
 * sensor node, a complex IoT device, or a multi-functional hub, the `CORE`
 * remains the mandatory backbone. Even when using high-level libraries on top
 * of it, the `CORE` continues to manage the system lifecycle and does not
 * allow its essential functions to be overridden, ensuring consistent
 * stability across all types of nodes.
 */

/**
 * @defgroup config_c Core Entry
 * @brief Base initialization and framework-wide settings.
 *
 * This module contains functions that must be called at the very beginning
 * of the application to prepare internal data structures and system tasks.
 * @{
 */
/**
 * @brief Initializes the `CORE` of the framework.
 *
 * Sets up the internal configuration, prepares the web server settings, and
 * launches essential system services.
 *
 * @note This function must be called before any other `eif_*` functions
 *       (like URI registration or Wi-Fi connectivity). This function is
 *       **mandatory** for the framework to work.
 *
 * @warning Calling this function overwrites any parameters previously
 *          set via `eif_*` API with default values.
 *
 * @return
 *    - `ESP_OK`:         `CORE` initialization was successful.
 *    - `ESP_ERR_NO_MEM`: This error **cannot be returned**, because the system
 *                        automatically restarts due to the lack of a heap,
 *                        even when the memory scanner is running.
 *
 * Example of use:
 * @code{c}
 * #include <esp_err.h>
 * #include <esp_iot_framework_core.h>
 *
 * void app_main(void) {
 *     ESP_ERROR_CHECK(eif_core_initialize());
 *
 *     // Further code...
 * }
 * @endcode
 */
esp_err_t eif_core_initialize(void);
/**
 * @brief Initialize Non-Volatile Storage (NVS) and framework data.
 *
 * This function performs the following essential steps:
 * - Initializes the NVS flash partition (with automatic repair/erase if
 *   corrupted).
 * - Prepares or loads @ref wifi_profiles_desc "Wi-Fi profiles" based on the
 *   configured count.
 * - Manages TLS credentials and authentication data (if enabled).
 *
 * @warning When the device is started for the first time, this function will
 *          reboot the device, before that, TLS credentials will be created.
 *          This is done to reduce the RAM consumption of the device.
 * 
 * @note This function must be called after `eif_core_initialize()` but before
 *       `eif_wifi_initialize()`. This function is
 *       **mandatory** for the framework to work. If you use
 *       `eif_set_wifi_profiles_count()`, it must be called before calling
 *       `eif_nvs_initialize()`. Otherwise, the default number of profiles
 *       **(2)** will be loaded into NVS.
 *
 * @warning If the NVS partition is corrupted or has a new version, this function
 *          will automatically erase it and re-initialize, which results in the
 *          loss of previously stored data.
 *
 * @return
 *    - `ESP_OK`:    NVS and all framework fields initialized successfully.
 *    - `ESP_ERR_*`: Various NVS-related errors if hardware initialization fails.
 *                   Look at the logs to understand the cause of the errors.
 *
 * Example of use:
 * @code{c}
 * #include <esp_err.h>
 * #include <esp_iot_framework_core.h>
 *
 * void app_main(void) {
 *     ESP_ERROR_CHECK(eif_core_initialize());
 *     ESP_ERROR_CHECK(eif_nvs_initialize());
 *
 *     // Further code...
 * }
 * @endcode
 */
esp_err_t eif_nvs_initialize(void);
/** @} */



/**
 * @defgroup core_handlers Event Handlers
 * @brief System hooks for application business logic.
 * @{
 *
 * These functions are the primary interface for linking the final product's
 * business logic with system lifecycle events. They allow the application to
 * execute specific tasks in direct response to internal state changes.
 */
/**
 * @brief Prototype for system-level lifecycle event handlers.
 */
typedef esp_err_t (*eif_handler_system_t)(void);
/**
 * @brief Register a handler for the system reboot event.
 *
 * This function hooks a custom handler into the system teardown process.
 * It is the final safety net that triggers right before the system reset,
 * allowing you to "park" your hardware or commit last-second logs.
 *
 * @warning The handler runs within a dedicated reboot task with a limited
 *          stack size (`CONFIG_EIF_REBOOT_TASK_STACK_SIZE`). Avoid deep
 *          recursion or allocation of a large amount of memory (stack or heap).
 *
 * @note Before this handler runs, the framework will automatically trigger the
 *       "IP Lost" (registered via `eif_register_handler_ip_lost()`, ESP-IDF
 *       event: `IP_EVENT_STA_LOST_IP`) event and disable the Wi-Fi stack. This
 *       function must be called after `eif_core_initialize()`.
 *
 * @warning This registration is not cumulative. Any previously registered
 *          handler for this event will be overwritten. Only the most recent
 *          assignment is stored and executed by the `CORE`.
 *
 * @param handler Pointer to the function to execute. Cannot be `NULL`.
 *
 * @return
 *    - `ESP_OK`:              Handler registered successfully.
 *    - `ESP_ERR_INVALID_ARG`: The `handler` pointer is `NULL`.
 *
 * Example of use:
 * @code{c}
 * #include <esp_err.h>
 * #include <esp_iot_framework_core.h>
 *
 * esp_err_t my_reboot_logic(void) {
 *     // Emergency state save or hardware shutdown
 *     return ESP_OK;
 * }
 *
 * void app_main(void) {
 *     ESP_ERROR_CHECK(eif_core_initialize());
 *     ESP_ERROR_CHECK(eif_register_handler_system_reboot(my_reboot_logic));
 *    
 *     // Further code...
 * }
 * @endcode
 */
esp_err_t eif_register_handler_system_reboot(eif_handler_system_t handler);
/** @} */



/**
 * @defgroup core_consts Constants
 * @brief Default values and length limits.
 * @{
 *
 * This module contains default values and limits for Wi-Fi, HTTP Basic Auth,
 * and other system configuration parameters.
 */
/**
 * @name Wi-Fi Configuration
 * @brief Default values and validation limits for network profiles.
 * @{
 */
/**
 * @brief Maximum number of custom @ref wifi_profiles_desc "Wi-Fi profiles".
 */
#define EIF_WIFI_PROFILES_MAX_COUNT 9U
/**
 * @brief Default number of custom @ref wifi_profiles_desc "Wi-Fi profiles".
 */
#define EIF_WIFI_PROFILES_DEFAULT_COUNT 2U
/** @} */
/** @} */



 /**
 * @defgroup eif_wifi_config Wi-Fi Configuration
 * @brief Pre-initialization tuning and profile management for the Wi-Fi stack.
 *
 * This module provides fine-grained control over the Wi-Fi stack behavior,
 * including power management and connection recovery policies.
 * @{
 */
/**
 * @brief Configure Wi-Fi driver and power management policies.
 *
 * Overrides the default low-level driver settings. These parameters are
 * applied globally and define the performance and energy profile of the device.
 *
 * @note This function must be called after `eif_core_initialize()` but before
 *       `eif_wifi_initialize()`.
 *
 * @warning The function does not validate incoming data, which may result in
 *          unexpected behavior (`UB`), memory leaks, or Wi-Fi crashes. Always
 *          validate the data you send.
 *
 * @param wifi_driver_config Low-level ESP-IDF Wi-Fi driver configuration
 *                           Use `WIFI_INIT_CONFIG_DEFAULT()` as a base.
 * @param wifi_power_mode Wi-Fi Modem-sleep policy. Defines the trade-off between
 *                        power consumption and network responsiveness/latency.
 * @param wifi_attempt_delay_ms Delay in milliseconds before switching to
 *                              the next profile after a failed connection.
 *
 * @return
 *    - `ESP_OK`:              Settings saved successfully.
 *    - `ESP_ERR_INVALID_ARG`: The `wifi_driver_config` pointer is `NULL`.
 *
 * Example of use:
 * @code{c}
 * #include <esp_err.h>
 * #include <esp_wifi.h>
 * #include <esp_iot_framework_core.h>
 *
 * void app_main(void) {
 *     ESP_ERROR_CHECK(eif_core_initialize());
 *     ESP_ERROR_CHECK(eif_nvs_initialize());
 *
 *     // Creating default Wi-Fi settings
 *     wifi_init_config_t cfg_wifi = WIFI_INIT_CONFIG_DEFAULT();
 *     // Changing Wi-Fi settings
 *     cfg_wifi.dynamic_rx_buf_num = 64;
 *     cfg_wifi.csi_enable = false;
 *
 *     ESP_ERROR_CHECK(eif_set_wifi_config(&cfg_wifi, WIFI_PS_MIN_MODEM, 1000));
 *
 *     // Further code...
 * }
 * @endcode
 */
esp_err_t eif_set_wifi_config(
    const wifi_init_config_t *wifi_driver_config,
    wifi_ps_type_t wifi_power_mode,
    uint32_t wifi_attempt_delay_ms
);
/**
 * @brief Set the number of additional user Wi-Fi profiles.
 *
 * Defines how many additional user-defined slots are available in NVS.
 *
 * @note This function must be called after `eif_core_initialize()` but before
 *       `eif_nvs_initialize()`.
 *
 * @anchor wifi_profiles_desc
 * `Profile` - configuration for a specific Wi-Fi network (`SSID` and `Password`).
 *             Profiles are managed in a circular (ring) list: if a connection
 *             attempt fails or if the active connection is broken, the framework
 *             automatically rotates to the next available slot. Upon reaching
 *             the end of the list, it wraps around to the first profile. This
 *             cycle continues indefinitely until a stable connection is
 *             re-established.
 *
 * @note The total number of managed profiles is equal to
 *       (`wifi_profiles_count + 1`). If set to `0`, only the system default
 *       profile (index 0) will be used. The default profile is hardcoded and
 *       cannot be changed. The default value is
 *       <code>#EIF_WIFI_PROFILES_DEFAULT_COUNT</code>.
 *
 * @param wifi_profiles_count Number of extra slots (0 to
 *                            <code>#EIF_WIFI_PROFILES_MAX_COUNT</code>).
 *
 * @return
 *    - `ESP_OK`:              Profile capacity set and state reset.
 *    - `ESP_ERR_INVALID_ARG`: `wifi_profiles_count` exceeds
 *                             <code>#EIF_WIFI_PROFILES_MAX_COUNT</code>.
 *
 *
 * Example of use:
 * @code{c}
 * #include <esp_err.h>
 * #include <esp_iot_framework_core.h>
 *
 * void app_main(void) {
 *     ESP_ERROR_CHECK(eif_core_initialize());
 *     ESP_ERROR_CHECK(eif_set_wifi_profiles_count(5));
 *     ESP_ERROR_CHECK(eif_nvs_initialize());
 *
 *     // Further code...
 * }
 * @endcode
 */
esp_err_t eif_set_wifi_profiles_count(uint8_t wifi_profiles_count);
/**
 * @brief Launch the Wi-Fi subsystem and automated network services.
 *
 * Initializes the Wi-Fi stack, registers event handlers for automatic profile
 * failover, and starts mDNS/Web Server once an IP is obtained. In case of
 * IP loss, the Web Server and mDNS services are gracefully stopped to save
 * resources and prevent invalid states. Once a new IP address is obtained,
 * the framework automatically resumes these services.
 *
 * @note This function should come after any other `eif_*` functions
 *       (like URI registration or Wi-Fi connectivity). This function is
 *       **mandatory** for the framework to work.
 *
 * @warning Otherwise, there is a chance that the settings will not be applied
 *          immediately or unexpected behavior (`UB`) will occur.
 *
 * @return
 *    - `ESP_OK`:         Subsystem initialized successfully.
 *    - `ESP_ERR_NO_MEM`: Failed to allocate memory for network interface.
 *    - `ESP_ERR_*`:      Internal driver errors. Look at the logs to understand
 *                        the cause of the errors.
 *
 * Example of use:
 * @code{c}
 * #include <esp_err.h>
 * #include <esp_iot_framework_core.h>
 *
 * void app_main(void) {
 *     ESP_ERROR_CHECK(eif_core_initialize());
 *     ESP_ERROR_CHECK(eif_nvs_initialize());
 *     ESP_ERROR_CHECK(eif_wifi_initialize());
 *    
 *     // Necessary to keep the flow alive
 *     while (1) {
 *         vTaskDelay(pdMS_TO_TICKS(1000));
 *     }
 * }
 * @endcode
 */
esp_err_t eif_wifi_initialize(void);
/** @} */
/** @} */

#ifdef __cplusplus
    }
#endif
#endif