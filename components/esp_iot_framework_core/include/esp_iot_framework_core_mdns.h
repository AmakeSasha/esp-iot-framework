/*
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Folder: ./components/esp_iot_framework_core/include
 * File: esp_iot_framework_core_mdns.h
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

#ifndef ESP_IOT_FRAMEWORK_CORE_MDNS_H
#define ESP_IOT_FRAMEWORK_CORE_MDNS_H

#include "sdkconfig.h"
#if defined(CONFIG_EIF_ENABLE_MDNS) || defined(DOXYGEN)
    #include <mdns.h>
#endif

#ifdef __cplusplus
    extern "C" {
#endif

#if defined(CONFIG_EIF_ENABLE_MDNS) || defined(DOXYGEN)
    /**
     * @addtogroup core_group CORE
     * @{
     */

    /**
     * @defgroup eif_mdns mDNS Configuration
     * @brief Network discovery and mDNS management
     *
     * @note This module is available when `Kconfig` option <code>
     *         <a href="group__core__kconfig.html#CONFIG_EIF_ENABLE_MDNS">
     *           CONFIG_EIF_ENABLE_MDNS
     *         </a>
     *       </code> is enabled and you include this line at the beginning of the file.:
     *       @code{c}
     *       #include <esp_iot_framework_core_mdns.h>
     *       @endcode
     *
     * This module allows you to configure mDNS. mDNS enables the device to be
     * discovered via a human-readable name (e.g., `my-device.local`) instead of an
     * IP address.
     *
     * The framework automatically appends a unique identifier based on
     * the MAC address to prevent network collisions (e.g., common hostname set:
     * `esp32.local`, the framework turns this into `esp32-1a2b3c.local` on a
     * device with MAC `XX:XX:XX:1A:2B:3C`).
     *
     * @note In case the mDNS hostname is not set, `device` will be used as the
     *       common hostname. To disable mDNS, turn off `Kconfig` option <code>
     *         <a href="group__core__kconfig.html#CONFIG_EIF_ENABLE_MDNS">
     *           CONFIG_EIF_ENABLE_MDNS
     *         </a>
     *       </code>.
     *
     * If you frequently need to enable and disable mDNS during development,
     * you can use the following construct:
     * @code{c}
     * #include <mdns.h>
     * #include <esp_err.h>
     * #include <esp_iot_framework_core.h>
     * #ifdef CONFIG_EIF_ENABLE_MDNS
     *     #include <esp_iot_framework_core_mdns.h>
     * #endif
     *
     * // Not necessary, but nice :)
     * #ifdef CONFIG_EIF_ENABLE_MDNS
     *     static const mdns_txt_item_t my_txt_records[] = {
     *         {"friendly_name", "Kitchen Main Light"},
     *         {"room",          "Kitchen"},
     *         {"hardware",      "ESP32-S3-DevKit"},
     *         {"v",             "1.0.4"}
     *     };
     * #endif
     *
     * void app_main(void) {
     *     ESP_ERROR_CHECK(eif_core_initialize());
     *     ESP_ERROR_CHECK(eif_nvs_initialize());
     *
     *     #ifdef CONFIG_EIF_ENABLE_MDNS
     *         ESP_ERROR_CHECK(eif_set_mdns("my-esp", "Smart Controller"));
     *         ESP_ERROR_CHECK(eif_set_mdns_records(my_txt_records, 4));
     *     #endif
     *
     *     // Further code...
     * }
     * @endcode
     * @{
     */

    /**
     * @brief Maximum number of mDNS txt-records
     */
    #define EIF_MDNS_TXT_RECORDS_MAX_COUNT 32U
    /**
     * @brief Maximum length of the mDNS hostname prefix including null-terminator (`\0)`.
     */
    #define EIF_MDNS_HOSTNAME_MAX_LEN 57U
    /**
     * @brief Maximum length of the mDNS instance name (friendly name) including null-terminator (`\0)`.
     */
    #define EIF_MDNS_INSTANCE_NAME_MAX_LEN 64U

    /**
     * @brief Configure the mDNS hostname and instance name.
     *
     * Sets the base identity of the device on the local network.
     *
     * @note This function must be called after `eif_core_initialize()` but before
     *       `eif_wifi_initialize()`.
     *
     * @param hostname Base name prefix (e.g., `my-sensor`). The length should be less
     *                 than <code>#EIF_MDNS_HOSTNAME_MAX_LEN</code>. If empty,
     *                 the default hostname will be used (`device`).
     * @param instance_name Friendly name for discovery tools (e.g., `Hall
     *                      Sensor`). The length should be less than 
     *                      <code>#EIF_MDNS_INSTANCE_NAME_MAX_LEN</code>. If empty,
     *                      the formatted `mdns_hostname` (with MAC) will be used as
     *                      the instance name by default. 
     *
     * @return
     *    - `ESP_OK`:               Configuration stored.
     *    - `ESP_ERR_INVALID_ARG`:  The pointer `hostname` or `instance_name`
     *                              is `NULL`.
     *    - `ESP_ERR_INVALID_SIZE`: The length of the `hostname` or
     *                              `instance_name` is outside the
     *                              acceptable range.
     *
     * Example of use:
     * @code{c}
     * #include <esp_err.h>
     * #include <esp_iot_framework_core.h>
     * #include <esp_iot_framework_core_mdns.h>
     *
     * void app_main(void) {
     *     ESP_ERROR_CHECK(eif_core_initialize());
     *     ESP_ERROR_CHECK(eif_nvs_initialize());
     *     ESP_ERROR_CHECK(eif_set_mdns("my-esp", "Smart Controller"));
     *
     *     // Further code...
     * }  
     * @endcode
     */
    esp_err_t eif_set_mdns(
        const char * const hostname, const char * const instance_name
    );

    /**
     * @brief Set custom TXT records for mDNS service discovery.
     *
     * TXT records allow you to broadcast additional metadata about the device
     * (e.g., firmware version, model, or status). The framework creates an
     * internal copy of the structure, so the original data's lifecycle no
     * longer matters after the call.
     *
     * @note This function must be called after `eif_core_initialize()` but before
     *       `eif_wifi_initialize()`.
     *
     * @warning The handler array length must be equal to `txt_records_count`. If
     *          the count exceeds the actual array length, a `Buffer Overread` will
     *          occur. If the count is less than the actual length, error
     *          `ESP_ERR_INVALID_ARG` will return.
     *          <br>
     *          If the array is accessible by value at the function call site, you
     *          can use the following code to automatically calculate the length:
     *          @code{c}
     *          sizeof(array) / sizeof(array[0])
     *          @endcode
     *
     * @param txt_records       Array of mDNS TXT items. The length should be
     *                          <code>#EIF_MDNS_TXT_RECORDS_MAX_COUNT</code>.
     * @param txt_records_count Number of elements in the array.
     *
     * @return
     *    - `ESP_OK`:               Records applied.
     *    - `ESP_ERR_INVALID_ARG`:  The pointer `key` or `value` from
     *                              `txt_records` is `NULL`.
     *    - `ESP_ERR_INVALID_SIZE`: The `txt_records` length is less than
     *                              `txt_records_count`.
     *
     * Example of use:
     * @code{c}
     * #include <mdns.h>
     * #include <esp_err.h>
     * #include <esp_iot_framework_core.h>
     * #include <esp_iot_framework_core_mdns.h>
     *
     * static const mdns_txt_item_t my_txt_records[] = {
     *     {"friendly_name", "Kitchen Main Light"},
     *     {"room",          "Kitchen"},
     *     {"hardware",      "ESP32-S3-DevKit"},
     *     {"v",             "1.0.4"}
     * };
     *
     * void app_main(void) {
     *     ESP_ERROR_CHECK(eif_core_initialize());
     *     ESP_ERROR_CHECK(eif_nvs_initialize());
     *     ESP_ERROR_CHECK(eif_set_mdns_records(my_txt_records, 4));
     *
     *     // Further code...
     * }
     * @endcode
     */
    esp_err_t eif_set_mdns_records(
        const mdns_txt_item_t txt_records[EIF_MDNS_TXT_RECORDS_MAX_COUNT],
        size_t txt_records_count
    );
    /** @} */
    /** @} */

    /**
     * @addtogroup core_ext_group CORE Extension
     * @{
     */

    /**
     * @defgroup eif_mdns_ext mDNS Extension
     * @brief Network discovery and mDNS management
     *
     * @note This module is available when `Kconfig` option <code>
     *         <a href="group__core__kconfig.html#CONFIG_EIF_ENABLE_MDNS">
     *           CONFIG_EIF_ENABLE_MDNS
     *         </a>
     *       </code> is enabled and you include this line at the beginning of the file.:
     *       @code{c}
     *       #include <esp_iot_framework_core_mdns.h>
     *       @endcode
     *
     * This module provides more advanced and low-level tools for working with mDNS.
     * @{
     */

    /**
     * @brief Get the currently configured mDNS hostname.
     *
     * Returns the device network name used for local mDNS domain routing.
     * 
     * @warning The returned pointer references the immutable internal configuration 
     *          buffer owned by the core module. Do **NOT** attempt to explicitly
     *          `free()` this pointer, and do **NOT** modify its contents via type
     *          casting. Doing so will corrupt the core memory or cause an immediate
     *          hardfault.
     * 
     * @return
     *    - `const char*`: Pointer to the null-terminated hostname string.
     *    - `NULL`:        mDNS is disabled via <code>
     *                       <a href="group__core__kconfig.html#CONFIG_EIF_ENABLE_MDNS">
     *                         CONFIG_EIF_ENABLE_MDNS
     *                       </a>
     *                     </code>.
     * 
     * Example of use:
     * @code{c}
     * #include <esp_iot_framework_core_mdns.h>
     *
     * void some_function(void) {
     *     const char *hostname = eif_core_get_mdns_hostname();
     *     if (hostname != NULL) {
     *         // Use hostname...
     *     } else {
     *         // mDNS is disabled...
     *     }
     * }  
     * @endcode
     */
    const char* eif_get_mdns_hostname(void);
    /** @} */
    /** @} */
#elif defined(CONFIG_EIF_ENABLE_TLS)
    /* esp32-divece */
    #define MDNS_HOSTNAME_PREFIX_MAX_LEN MDNS_HOSTNAME_FULL_MAX_LEN - MDNS_SUFFIX_MAC_LEN
#endif

#ifdef __cplusplus
    }
#endif
#endif