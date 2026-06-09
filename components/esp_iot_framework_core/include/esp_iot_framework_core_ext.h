/* SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Library: esp_iot_framework_core
 * Folder: components/esp_iot_framework_core/include
 * File: esp_iot_framework_core_ext.h
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

#ifndef ESP_IOT_FRAMEWORK_CORE_EXT_H 
#define ESP_IOT_FRAMEWORK_CORE_EXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/**
 * @addtogroup core_ext_group Core Extension
 * @{
 * 
 * @details @note This group of modules is available when you include this line
 * at the beginning of the file.:
 * @code{c}
 * #include "esp_iot_framework_core_ext.h"
 * @endcode
 * 
 * This API provides direct access to the CORE internal subsystems. It
 * is designed for building specialized nodes that require low-level system event
 * orchestration, raw configuration access, and manual NVS storage management.
 * 
 * @warning Operating at this level bypasses standard safety abstractions. 
 * Improper use can lead to resource contention, deadlocks, or storage 
 * corruption.
 */



/**
 * @defgroup core_ext_consts Constants
 * @brief Default values and length limits.
 * @{
 * 
 * This module contains default values and limits for Wi-Fi, HTTP Basic Auth,
 * and other system configuration parameters.
 */

/**
 * @name Wi-Fi Configuration
 * @brief Default values and validation limits for @ref wifi_profiles_desc "Wi-Fi profiles".
 * @{ */
/** @brief Default `SSID` value. */
#define EIF_WIFI_SSID_DEFAULT "ESP32_SETUP"
/** @brief Minimum `SSID` length without null-terminator.<br>Example of the value: `Q` */
#define EIF_WIFI_SSID_MIN_LEN 1U
/** @brief Maximum `SSID` length including null-terminator.<br>Example of the value: `My_Super_Long_Wifi_Network_Name` */
#define EIF_WIFI_SSID_MAX_LEN 32U

/** @brief Default `Password` value. */
#define EIF_WIFI_PASS_DEFAULT "12345678"
/** @brief Minimum `Password` length for WPA2 without null-terminator.<br>Example of the value: `12345678` */
#define EIF_WIFI_PASS_MIN_LEN 8U
/** @brief Maximum `Password` length including null-terminator.<br>Example of the value: `this_is_a_very_long_password_that_exactly_reaches_63_characters` */
#define EIF_WIFI_PASS_MAX_LEN 64U
/** @} */

/** 
 * @name Basic Auth Configuration 
 * @brief Validation limits for HTTP Basic Auth.
 * @{ */
/** @brief Minimum `Line` length without null-terminator.<br>Example of the value: `Basic YWRtaW46` */
#define EIF_BASIC_AUTH_LINE_MIN_LEN 14U
/** @brief Maximum `Line` length including null-terminator.<br>Example of the value: `Basic YWRtaW46MTIzNDU2Nzg5MHF3ZXJ0eTBxd2VydHkwcXdlcnR5MDE=` */
#define EIF_BASIC_AUTH_LINE_MAX_LEN 59U
/** @brief Minimum `Password` length without null-terminator.<br>Example of the value: <code>ㅤ</code>*/
#define EIF_BASIC_AUTH_PASS_MIN_LEN 0U
/** @brief Maximum `Password` length including null-terminator.<br>Example of the value: `1234567890qwerty0qwerty0qwerty01` */
#define EIF_BASIC_AUTH_PASS_MAX_LEN 33U
/** @} */

/** 
 * @name NVS Storage Keys 
 * @brief The keys used by the NVS module to access the values.
 * @{ */
/** @brief TLS certificate blob. */
#define EIF_NVS_KEY_TLS_CERT "eif_tls_cert"
/** @brief TLS private key blob. */
#define EIF_NVS_KEY_TLS_PRIV_KEY "eif_tls_key"
/** @} */

/** @} */



/**
 * @defgroup core_ext_tools Tools
 * @brief Helper utility functions.
 * 
 * @{
 * 
 * This module contains inline helper utilities designed for secure and 
 * defensive manipulation of strings, preventing common buffer overflows and
 * null-pointer dereferences.
 */

/**
 * @brief Checks if a string is empty.
 * 
 * Evaluates the provided string to determine if it is either a `NULL` 
 * pointer or points directly to a null-terminator (`\0`).
 * 
 * @param str Pointer to the string. Can be `NULL`.
 * 
 * @return
 *    - `true`:  The string pointer is `NULL` or the string is empty (`""`).
 *    - `false`: The string is valid and contains at least one character.
 * 
 * Example of use:
 * @code{c}
 * #include "esp_log.h"
 * #include "esp_iot_framework_code_ext.h"
 * 
 * #define TAG "STR_UTIL"
 * 
 * void validate_json_payload(const char * const payload) {
 *     const bool is_empty = eif_strempty(payload);
 *     if (is_empty == true) {
 *         ESP_LOGE(TAG, "Payload is completely empty or NULL.");
 *     } else {
 *         ESP_LOGI(TAG, "Payload validation passed.");
 *     }
 * }
 * @endcode
 */
static inline bool eif_strempty(const char * const str) {
    return ((str == NULL) || (str[0] == '\0'));
}

/**
 * @brief Computes the length of string.
 * 
 * Calculates the length of the string `str`, up to a maximum allowed bound
 * specified by `max_allowed`.
 * 
 * @param str         Constant pointer to the constant string to measure.
 * @param max_allowed Maximum number of characters to examine, excluding the
 *                    null-terminator.
 * 
 * @return 
 *    * The number of characters in `str` if a null-terminator is found within
 *      the boundary. If `str` is `NULL`, returns `0U`. If no null-terminator
 *      is found within the limit, returns the internal bounded limit.
 * 
 * Example of use:
 * @code{c}
 * #include "esp_log.h"
 * #include "esp_iot_framework_code_ext.h"
 * 
 * #define TAG "STR_UTIL"
 * #define MAX_BUFFER_SIZE 64U
 * 
 * void check_string_bounds(const char * const user_input) {
 *     const size_t calculated_len = eif_strnlen(user_input, MAX_BUFFER_SIZE);
 *     
 *     if (calculated_len > MAX_BUFFER_SIZE) {
 *         ESP_LOGE(TAG, "Input string exceeds maximum allowed limit of %u.", MAX_BUFFER_SIZE);
 *     } else {
 *         ESP_LOGI(TAG, "String length is safe: %zu bytes.", calculated_len);
 *     }
 * }
 * @endcode
 */
static inline size_t eif_strnlen(const char * const str, size_t max_allowed) {
    size_t search_limit = 0U;
    if (str != NULL) {
        if (max_allowed < SIZE_MAX) {
            search_limit = max_allowed + 1U;
        } else {
            search_limit = max_allowed;
        }
    }

    /* Cleanup */
    /* Required for the new compiler 'GCC 11+' (ESP-IDF v5+) */
    #if defined(__GNUC__) && (__GNUC__ >= 11)
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wstringop-overread"
    #endif
        /* @deviation [Rule 21.18] The search limit is set to (max_allowed + 1) 
         * where possible to allow the caller to distinguish between a string 
         * that exactly fits the limit and one that exceeds it. Pre-check 
         * against 'SIZE_MAX' ensures no integer wrap-around occurs. Memory 
         * access is guaranteed to be within safe bounds as strnlen stops at
         * the first null-terminator or the provided 'search_limit'. */
        return strnlen(str, search_limit);
    #if defined(__GNUC__) && (__GNUC__ >= 11)
        #pragma GCC diagnostic pop
    #endif
}
/** @} */



/**
 * @defgroup core_ext_config Configuration
 * @brief Functions to retrieve current system configuration.
 * 
 * @{
 * 
 * This module aggregates all functions used to query current 
 * operational flags, device parameters, and active configuration blocks.
 * 
 * @note Before calling any function in this group, `eif_core_initialize()` 
 *       must be executed exactly once.<br><br>
 *       Examples: 
 *       @code{c}
 *       #include "esp_err.h"
 *       #include "esp_iot_framework_core.h"
 *       
 *       void app_main(void) {
 *           ESP_ERROR_CHECK(eif_core_initialize());
 *           
 *           // Further code...
 *       }
 *       @endcode
 */

/**
 * @brief Wi-Fi profile availability test result.
 *
 * Stores the outcome of a network availability check for a specific Wi-Fi profile. 
 * This structure captures whether the target network is reachable, accepts 
 * credentials, and successfully provides a valid IP address.
 */
typedef struct {
    /**
     * @brief Network availability status.
     * 
     * `true` if the network is reachable, credentials are valid, and an IP 
     * address is obtained within the timeout. `false` if the network is 
     * unavailable or the connection fails.
     */
    bool connected;
    /**
     * @brief Received Signal Strength Indicator (RSSI).
     * 
     * Measured in dBm during the availability check. Defaults to `-127` 
     * if the network is unreachable or the connection attempt fails.
     */
    int8_t rssi;
} eif_wifi_test_result;

/**
 * @brief Gets the total number of @ref wifi_profiles_desc "Wi-Fi profiles".
 * 
 * Returns the number of @ref wifi_profiles_desc "Wi-Fi profiles" stored in
 * the system. It is set using `eif_set_wifi_profiles_count()`.
 * 
 * Example of use:
 * @code{c}
 * #include "esp_log.h"
 * #include "esp_iot_framework_code_ext.h"
 * 
 * #define TAG "WIFI_CFG"
 * 
 * void check_profiles_bound(void) {
 *     size_t total_profiles = eif_wifi_get_profiles_count();
 *     ESP_LOGI(TAG, 
 *         "Framework is currently managing %zu Wi-Fi profile(s).",
 *         total_profiles);
 * }
 * @endcode
 */
uint8_t eif_wifi_get_profiles_count(void);

/**
 * @brief Gets the index of the currently active @ref wifi_profiles_desc "Wi-Fi profile".
 * 
 * Returns the zero-based index of the active 
 * @ref wifi_profiles_desc "Wi-Fi profile" currently in use.
 * 
 * Example of use:
 * @code{c}
 * #include "esp_log.h"
 * #include "esp_iot_framework_code_ext.h"
 * 
 * #define TAG "WIFI_CFG"
 * 
 * void log_active_profile_index(void) {
 *     uint8_t active_idx = eif_wifi_get_current_profile_index();
 *     ESP_LOGI(TAG,
 *         "Subsystem is currently operating on profile index: %u",
 *         active_idx);
 * }
 * @endcode
 */
uint8_t eif_wifi_get_current_profile_index(void);

/**
 * @brief Gets the network availability test result for a specific Wi-Fi profile.
 * 
 * Copies the latest availability check result for the specified profile `index` 
 * into the `out_result` structure.
 * 
 * @param index      Zero-based Wi-Fi profile index to query. 
 *                   Must be less than `eif_wifi_get_profiles_count()`.
 * @param out_result Pointer to the structure where the result will be copied. 
 *                   Cannot be `NULL`.
 * 
 * @return
 *    - `ESP_OK`:               Result was successfully fetched and copied.
 *    - `ESP_ERR_INVALID_ARG`:  The `out_result` pointer is `NULL`.
 *    - `ESP_ERR_INVALID_SIZE`: The `index` is out of the possible range.
 * 
 * Example of use:
 * @code{c}
 * #include "esp_log.h"
 * #include "esp_err.h"
 * #include "esp_iot_framework_wifi.h"
 * 
 * #define TAG "APP_WIFI"
 * 
 * void check_profile_status(uint8_t profile_idx) {
 *     eif_wifi_test_result result = {0};
 * 
 *     esp_err_t err = eif_wifi_get_test_result(profile_idx, &result);
 *     if (err == ESP_OK) {
 *         ESP_LOGI(TAG, "Profile %u: Connected = %s, RSSI = %d dBm", 
 *                  profile_idx, result.connected ? "YES" : "NO", result.rssi);
 *     } else if (err == ESP_ERR_INVALID_SIZE) {
 *         ESP_LOGE(TAG, "Profile index %u is out of bounds", profile_idx);
 *     } else {
 *         ESP_LOGE(TAG, "Failed to get test result: %s", esp_err_to_name(err));
 *     }
 * }
 * @endcode
 */
esp_err_t eif_wifi_get_test_result(
    uint8_t index, eif_wifi_test_result * const out_result
);
/** @} */



/**
 * @defgroup core_ext_handlers Event Handlers
 * @brief System hooks for internal business logic.
 * @{
 * 
 * These functions are the primary interface for linking the final product's 
 * business logic with system-wide lifecycle events. They allow the application 
 * to execute specific tasks in direct response to internal state transitions, 
 * hardware updates, and subsystem status changes.
 * 
 * @note Before calling any function in this group, `eif_core_initialize()` 
 *       must be executed exactly once, and all calls must take place prior 
 *       to `eif_wifi_initialize()`.<br><br>
 *       Examples: 
 *       @code{c}
 *       #include "esp_err.h"
 *       #include "esp_iot_framework_core.h"
 *       
 *       void app_main(void) {
 *           ESP_ERROR_CHECK(eif_core_initialize());
 *           
 *           // Further code...
 * 
 *           ESP_ERROR_CHECK(eif_wifi_initialize());
 *       }
 *       @endcode
 */

/**
 * @name IP layer events
 * @{
 * 
 * These handlers are dispatched automatically by the framework when 
 * the corresponding IP-level event occurs.
 */

/**
 * @brief Prototype for network IP lifecycle event handlers.
 */
typedef esp_err_t (*eif_handler_ip_t)(void);

/**
 * @brief Register a handler for the `IP_EVENT_STA_GOT_IP`.
 *
 * Registers a callback for processing network layer events. It is the primary
 * trigger point that executes automatically as soon as the station secures a
 * valid IP lease from the DHCP server.
 * 
 * @warning This registration is not cumulative. Any previously registered 
 *          handler for this event will be overwritten. Only the most recent
 *          assignment is stored and executed by the `CORE`.
 * 
 * @param handler Pointer to the function to execute. Cannot be `NULL`.
 * 
 * @return 
 *    - `ESP_OK`: Handler registered successfully.
 *    - `ESP_ERR_INVALID_ARG`: The `handler` pointer is `NULL`.
 * 
 * Example of use:
 * @code{c}
 * #include "esp_err.h"
 * #include "esp_iot_framework_core.h"
 * #include "esp_iot_framework_core_ext.h"
 * 
 * esp_err_t ip_got_hanlder(void) {
 *     // Initialize HTTP server or cloud telemetry tasks safely here
 *     return ESP_OK;
 * }
 * 
 * void app_main(void) {
 *     ESP_ERROR_CHECK(eif_core_initialize());
 *     ESP_ERROR_CHECK(eif_register_handler_ip_got(ip_got_hanlder));
 *     
 *     // Further code...
 * }
 * @endcode
 */
esp_err_t eif_register_handler_ip_got(eif_handler_ip_t handler);

/**
 * @brief Register a handler for the `IP_EVENT_STA_LOST_IP`.
 *
 * @warning Due to the limitations of the SDK, the framework automatically
 *          calls this handler at event `WIFI_EVENT_STA_DISCONNECTED`, since
 *          it is equivalent to `IP_EVENT_STA_LOST_IP`. If `IP_EVENT_STA_LOST_IP`
 *          is separate, the handler will be called twice.
 * 
 * Registers a callback for processing network layer events. It executes 
 * automatically when the station drops its Wi-Fi connection, the AP kicks the
 * client, or the DHCP lease expires.
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
 * #include "esp_err.h"
 * #include "esp_iot_framework_core.h"
 * #include "esp_iot_framework_core_ext.h"
 * 
 * esp_err_t ip_lost_hanlder(void) {
 *     // Gracefully put cloud communication tasks to sleep
 *     return ESP_OK;
 * }
 * 
 * void app_main(void) {
 *     ESP_ERROR_CHECK(eif_core_initialize());
 *     ESP_ERROR_CHECK(eif_register_handler_ip_lost(ip_lost_hanlder));
 *     
 *     // Further code...
 * }
 * @endcode
 */
esp_err_t eif_register_handler_ip_lost(eif_handler_ip_t handler);
/** @} */

/** @} */



/**
 * @defgroup core_ext_nvs Non-Volatile Storage (NVS)
 * @brief NVS extension layer for data persistence and configuration profiles.
 * @{
 * 
 * This module provides high-level functions for data persistence in NVS. It
 * enforces runtime buffer protection, boundary validation, and key 
 * compatibility checks, completely preventing memory corruption and invalid
 * configurations within the framework.
 * 
 * @note Before calling any function in this group, `eif_core_initialize()` 
 *       and `eif_nvs_initialize()` must be executed exactly once.<br><br>
 *       Examples: 
 *       @code{c}
 *       #include "esp_err.h"
 *       #include "esp_iot_framework_core.h"
 *       #include "esp_iot_framework_core_ext.h"
 *       
 *       void app_main(void) {
 *           ESP_ERROR_CHECK(eif_core_initialize());
 *           ESP_ERROR_CHECK(eif_nvs_initialize());
 *           
 *           // Further code...
 *       }
 *       @endcode
 */

/**
 * @name String
 * @brief Functions for reading and writing strings to NVS with length validation.
 * @{
 */
/**
 * @brief Validates string length and writes it to NVS.
 * 
 * Writes the passed string (`value`) to NVS using the passed key (`key`). 
 * Before writing, the function checks whether the string length is in the
 * range from `min_len` to `max_len`.
 * 
 * Checking the length depending on `it_can_be_empty` (`len` it is length 
 * `value`):
 * - If `it_can_be_empty` is **true**: `((len >= min_len) && (len <= max_len)) || (len == 0)`
 * - If `it_can_be_empty` is **false**: `((len >= min_len) && (len <= max_len))`
 * 
 * @param key             NVS storage key (maximum `15` characters). Cannot be
 *                        `NULL`.
 * @param value           The string to save. Cannot be `NULL`.
 * @param min_len         Minimum allowed length (excluding the null-terminator).
 * @param max_len         Maximum allowed length (including the null-terminator).
 * @param it_can_be_empty Allow saving an empty string, bypassing length limits.
 * 
 * @return
 *    - `ESP_OK`:               String verified, written, and committed successfully.
 *    - `ESP_ERR_INVALID_ARG`:  The pointer `key` or `value` is `NULL`.
 *    - `ESP_ERR_INVALID_SIZE`: The length of the `value` is outside the
 *                              acceptable range.
 *    - `ESP_ERR_NVS_*`:        System errors propagated from `nvs_open()`, 
 *                              `nvs_set_str()`, or `nvs_commit()`.
 * 
 * Example of use:
 * @code{c}
 * #include "esp_log.h"
 * #include "esp_err.h"
 * #include "esp_iot_framework_core_ext.h"
 * 
 * #define TAG "CONFIG"
 * 
 * void save_dev_name(char *new_name) {
 *     // Save device name (min 3, max 31 chars, cannot be empty)
 *     esp_err_t err = eif_nvs_value_save("dev_name", new_name, 3, 32, false);
 * 
 *     if (err == ESP_OK) {
 *         ESP_LOGI(TAG, "Name saved!");
 *     } else {
 *         ESP_LOGE(TAG, "Failed to save: %s", esp_err_to_name(err));
 *     }
 * }
 * @endcode
 */
esp_err_t eif_nvs_value_save(
    const char * const key, const char * const value,
    size_t min_len, size_t max_len, bool it_can_be_empty
);

/**
 * @brief Loads a string from NVS into a pre-allocated buffer.
 * 
 * Reads the string associated with the `key` and copies it into `value_out`. 
 * The function prevents buffer overflows by forcing the native read operation
 * to respect the `max_len` limit, ensuring the output is safely stored within
 * the allocated boundaries.
 * 
 * @note If `ESP_ERR_NVS_NOT_FOUND` occurs and `max_len > 1U`, the `value_out[0]`
 *       is explicitly set to `\0` to ensure safety.
 * 
 * @warning `max_len` must match the length of `value_out`, as this requires
 *          `nvs_get_str()`. Otherwise, data may be truncated.
 * 
 * @param key       NVS storage key (maximum `15` characters). Cannot be `NULL`.
 * @param value_out Pointer to the buffer where the string will be stored. 
 *                  Cannot be `NULL`.
 * @param max_len   Maximum bytes allocated for `value_out` (including the 
 *                  null-terminator).
 * 
 * @return 
 *    - `ESP_OK`:                Value located and copied successfully.
 *    - `ESP_ERR_INVALID_ARG`:   The pointer `key` or `value_out` is `NULL`.
 *    - `ESP_ERR_NVS_NOT_FOUND`: The key does not exist in storage.
 *    - `ESP_ERR_NVS_*`:         System errors propagated from `nvs_open()` or 
 *                               `nvs_get_str()`.
 * 
 * Example of use:
 * @code{c}
 * #include "esp_log.h"
 * #include "esp_err.h"
 * #include "esp_iot_framework_core_ext.h"
 * 
 * #define TAG "CONFIG"
 * 
 * void load_dev_name(void) {
 *     char buffer[32] = {0};
 * 
 *     // Load saved name. If not found, buffer will be empty ("")
 *     esp_err_t ret = eif_nvs_value_load("dev_name", buffer, sizeof(buffer));
 * 
 *     if (ret == ESP_OK) {
 *         ESP_LOGI(TAG, "Loaded name: %s", buffer);
 *     } else if (ret == ESP_ERR_NVS_NOT_FOUND) {
 *         ESP_LOGW(TAG, "Name not found, using default.");
 *     } else {
 *         ESP_LOGE(TAG, "Error: %s", esp_err_to_name(ret));
 *     }
 * }
 * @endcode
 */
esp_err_t eif_nvs_value_load(
    const char * const key, char * const value_out, size_t max_len
);

/**
 * @brief Dynamically allocates memory and loads a string from NVS.
 * 
 * Queries storage to determine the data size, allocates a buffer from the heap
 * via `pvPortMalloc()`, and copies the string into it. If any operation fails,
 * `*value_out` is guaranteed to be set to `NULL` and `*value_out_len` is set
 * to `0U`.
 * 
 * @note This function automatically calls `vPortFree(*value_out)` at the 
 *       beginning to clear pre-existing allocations.
 * 
 * @warning The caller assumes ownership of the allocated buffer. If this
 *          function returns `ESP_OK`, the caller MUST release the memory using
 *          `vPortFree(*value_out)` (or the respective pointer variable) to
 *          avoid leaks.
 * 
 * @param key           NVS storage key (maximum `15` characters). Cannot be 
 *                      `NULL`.
 * @param value_out     Double pointer to store the allocated buffer address. 
 *                      Cannot be `NULL`.
 * @param value_out_len Pointer to store the string length (including the `\0`).
 *                      Cannot be `NULL`.
 * 
 * @return 
 *    - `ESP_OK`:                Memory allocated and string loaded successfully.
 *    - `ESP_ERR_INVALID_ARG`:   The pointer `key`, `value_out`, or
 *                               `value_out_len` is `NULL`.
 *    - `ESP_ERR_NVS_NOT_FOUND`: The key does not exist in storage.
 *    - `ESP_ERR_NO_MEM`:        Memory could not be allocated due to the lack
 *                               of an empty block of the required size.
 *    - `ESP_ERR_NVS_*`:         System errors propagated from `nvs_open()` or
 *                               `nvs_get_str()`.
 * 
 * Example of use:
 * @code{c}
 * #include "esp_log.h"
 * #include "esp_err.h"
 * #include "esp_iot_framework_core_ext.h"
 * 
 * #define TAG "CONFIG"
 * 
 * void load_dynamic_token(void) {
 *     char *token_buf = NULL; // Must be initialized to NULL
 *     size_t token_len = 0U;
 * 
 *     esp_err_t err = eif_nvs_value_load_malloc("huge_token", &token_buf, &token_len);
 * 
 *     if (err == ESP_OK) {
 *         ESP_LOGI(TAG, "Token loaded (%u bytes): %s", token_len, token_buf);
 *         
 *         // Mandatory resource cleanup
 *         vPortFree(token_buf);
 *         token_buf = NULL;
 *     } else if (err == ESP_ERR_NVS_NOT_FOUND) {
 *         ESP_LOGW(TAG, "Token not found.");
 *     } else {
 *         ESP_LOGE(TAG, "Failed to load token: %s", esp_err_to_name(err));
 *     }
 * }
 * @endcode
 */
esp_err_t eif_nvs_value_load_malloc(
    const char * key, char ** const value_out, size_t * const value_out_len
);
/** @} */

/**
 * @name Wi-Fi profile
 * @brief Functions for reading and writing Wi-Fi network credentials to NVS.
 * @{
 */
/**
 * @brief Saves Wi-Fi network credentials for a specific profile index.
 * 
 * Writes the provided SSID and password to NVS under auto-generated keys. 
 * Before writing, the function verifies that the profile index is within the
 * valid user range  and checks the credential string lengths.
 *
 * Valid index range check: `(index >= 1) && (index <= WIFI_PROFILES_MAX_COUNT)`
 * 
 * If both the ssid and the pass have a length of 0, the length checks are
 * ignored. This is done to clear the specified slot. The verification is
 * bypassed according to the following logic: 
 * `(len(ssid) == 0) && (len(pass) == 0)`
 * 
 * @note Profile index `0` is system-reserved (read-only). If you try to
 *       overwrite it, you will get an error.
 * 
 * @param index Profile index slot. Must be from `1` to 
 *              <code>@ref EIF_WIFI_PROFILES_MAX_COUNT</code>.
 * @param ssid  Wi-Fi SSID string. Cannot be `NULL`. The length should be from
 *              <code>@ref EIF_WIFI_SSID_MIN_LEN</code> to 
 *              <code>@ref EIF_WIFI_SSID_MAX_LEN</code>.
 * @param pass  Wi-Fi Password string. Cannot be `NULL`. The length should be
 *              from <code>@ref EIF_WIFI_PASS_MIN_LEN</code> to 
 *              <code>@ref EIF_WIFI_PASS_MAX_LEN</code>.
 * 
 * @return 
 *    - `ESP_OK`:               Profile validated and saved successfully.
 *    - `ESP_ERR_INVALID_ARG`:  The pointer `ssid` or `pass` is `NULL`.
 *    - `ESP_ERR_INVALID_SIZE`: The `index` is out of the possible range.
 *    - `ESP_ERR_NVS_*`:        System errors propagated from
 *                              `eif_nvs_value_save()`.
 * 
 * Example of use:
 * @code{c}
 * #include "esp_log.h"
 * #include "esp_err.h"
 * #include "esp_iot_framework_core_ext.h"
 * 
 * #define TAG "WIFI_CONFIG"
 * 
 * void save_user_network(void) {
 *     // Save user credentials to slot index 1
 *     esp_err_t err = eif_nvs_wifi_profile_save(1U, "MyHomeWiFi", "SecretPass123");
 * 
 *     if (err == ESP_OK) {
 *         ESP_LOGI(TAG, "Profile 1 stored successfully.");
 *     } else {
 *         ESP_LOGE(TAG, "Failed to store profile: %s", esp_err_to_name(err));
 *     }
 * }
 * @endcode
 */
esp_err_t eif_nvs_wifi_profile_save(
    uint8_t index, const char * const ssid, const char * const pass
);

/**
 * @brief Loads Wi-Fi network credentials for a specific profile index.
 * 
 * Reads SSID and password from NVS into pre-allocated buffers. 
 * 
 * Valid index range: `(index >= 0) && (index <= WIFI_PROFILES_MAX_COUNT)`
 * 
 * @note Querying profile 0 directly returns system-hardcoded credentials
 *       (<code>@ref EIF_WIFI_SSID_DEFAULT</code> and 
 *       <code>@ref EIF_WIFI_PASS_DEFAULT</code>), completely bypassing 
 *       flash memory access. Any other profile index
 *       explicitly queries the underlying NVS partition.
 * 
 * 
 * @param index    Profile index slot. Must be from `0` to 
 *                 <code>@ref EIF_WIFI_PROFILES_MAX_COUNT</code>.
 * @param ssid_out Buffer for SSID. Cannot be `NULL`. The length should be
 *              <code>@ref EIF_WIFI_SSID_MAX_LEN</code>.
 * @param pass_out Buffer for password. Cannot be `NULL`. The length should be
 *              <code>@ref EIF_WIFI_PASS_MAX_LEN</code>.
 * 
 * @return 
 *    - `ESP_OK`:                Profile loaded successfully.
 *    - `ESP_ERR_INVALID_ARG`:   The pointer `ssid_out` or `pass_out` is `NULL`.
 *    - `ESP_ERR_INVALID_SIZE`:  The `index` is out of the possible range.
 *    - `ESP_ERR_NVS_NOT_FOUND`: No data at specified index.
 *    - `ESP_ERR_NVS_*`:         System errors propagated from
 *                               `eif_nvs_value_load()`.
 * 
 * Example of use:
 * @code{c}
 * #include "esp_log.h"
 * #include "esp_err.h"
 * #include "esp_iot_framework_core_ext.h"
 * 
 * #define TAG "WIFI_CFG"
 * 
 * void load_profile(uint8_t idx) {
 *     char ssid[EIF_WIFI_SSID_MAX_LEN] = {0};
 *     char pass[EIF_WIFI_PASS_MAX_LEN] = {0};
 * 
 *     if (eif_nvs_wifi_profile_load(idx, ssid, pass) == ESP_OK) {
 *         ESP_LOGI(TAG, "Profile %u: SSID=[%s]", idx, ssid);
 *     }
 * }
 * @endcode
 */
esp_err_t eif_nvs_wifi_profile_load(
    uint8_t index, char * const ssid_out, char * const pass_out
);
/** @} */

/**
 * @name HTTP Basic Authentication
 * @brief Functions for reading, encoding, and writing Basic Auth credentials to NVS.
 * @{
 */
/**
 * @brief Encodes and saves the HTTP Basic Auth credentials to NVS.
 * 
 * Takes the raw password, combines it with the default username (`admin`), 
 * and encodes the `admin:password` combination into Base64. The final string 
 * is prefixed with <code>Basic&nbsp;</code> (e.g., `Basic YWRtaW46cGFzcw==`) and
 * saved to NVS.
 * 
 * If an empty password (length `0`) is provided, the function automatically 
 * generates and saves the default empty-password line (`Basic YWRtaW46`).
 * 
 * @param pass Raw password string. Cannot be `NULL`. The length should be from
 *             <code>@ref EIF_BASIC_AUTH_PASS_MIN_LEN</code>
 *             to <code>@ref EIF_BASIC_AUTH_PASS_MAX_LEN</code>.
 * 
 * @return 
 *    - `ESP_OK`:               Password successfully encoded, formatted, and
 *                              saved to NVS.
 *    - `ESP_ERR_INVALID_ARG`:  The `pass` pointer is `NULL`.
 *    - `ESP_ERR_INVALID_SIZE`: The length of the `pass` is outside the 
 *                              acceptable range.
 *    - `ESP_ERR_NO_MEM`:       Memory could not be allocated due to the lack 
 *                              of an empty block of the required size.
 *    - `ESP_ERR_NVS_*`:        System errors propagated from `eif_nvs_value_save()`.
 * 
 * @code{c}
 * #include "esp_log.h"
 * #include "esp_err.h"
 * #include "esp_iot_framework_core_ext.h"
 * #include "esp_iot_framework_core_macros.h"
 * 
 * #define TAG "AUTH_CFG"
 * 
 * void update_web_password(const unsigned char *new_pass) {
 *     EIF_TAG_WITH_UNUSED "AUTH";
 *     esp_err_t ret = ESP_OK;
 * 
 *     // Handles validation, Base64 encoding, and NVS storage automatically
 *     EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_nvs_basic_auth_line_save(new_pass),
 *         "Failed to save Basic Auth credentials");
 * 
 *     if (ret == ESP_OK) {
 *         ESP_LOGI(TAG, "Web authentication updated successfully.");
 *     }
 * }
 * @endcode
 */
esp_err_t eif_nvs_basic_auth_line_save(const unsigned char * const pass);

/**
 * @brief Loads the complete HTTP Basic Auth credentials from NVS.
 * 
 * Reads the stored authorization string from NVS into a pre-allocated buffer. 
 * The output string contains the configuration prefix, username, and password.
 * 
 * Example of output written to `buf_out`:
 * @code
 * Basic YWRtaW46bXlfcGFzcw==
 * @endcode
 * 
 * @param buf_out Buffer for Basic Auth string (e.g., `Basic YWRtaW46cGFzcw==`). 
 *                Cannot be `NULL`. The length should be
 *                <code>@ref EIF_BASIC_AUTH_LINE_MAX_LEN</code>.
 * 
 * @return 
 *    - `ESP_OK`:              Credential string located and loaded successfully.
 *    - `ESP_ERR_INVALID_ARG`: The `buf_out` pointer is `NULL`.
 *    - `ESP_ERR_NVS_*`:       System errors propagated from
 *                             `eif_nvs_value_load()`.
 * 
 * @code{c}
 * #include "esp_log.h"
 * #include "esp_err.h"
 * #include "esp_iot_framework_core_ext.h"
 * #include "esp_iot_framework_core_macros.h"
 * 
 * #define TAG "AUTH_RUN"
 * 
 * void apply_auth_header(void) {
 *     EIF_TAG_WITH_UNUSED "AUTH";
 *     esp_err_t ret = ESP_OK;
 *     char auth_line[EIF_BASIC_AUTH_LINE_MAX_LEN] = {0};
 * 
 *     EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_nvs_basic_auth_line_load(auth_line),
 *         "Failed to load auth line");
 * 
 *     if (ret == ESP_OK) {
 *         ESP_LOGI(TAG, "Loaded Auth Header: %s", auth_line);
 *         // Pass auth_line to HTTP client configuration here
 *     }
 * }
 * @endcode
 */
esp_err_t eif_nvs_basic_auth_line_load(char * const buf_out);
/** @} */

/** @} */



/**
 * @defgroup core_ext_tasks Task Management
 * @brief Thread lifecycle control and secure task spawn utilities.
 * @{
 * 
 * @details This group establishes a unified, deterministic framework for spawning 
 * and managing FreeRTOS tasks within the ecosystem. It enforces strict input 
 * verification, runtime state validation, and centralized resource allocation
 * boundaries. By formalizing task initialization paths, this subsystem eliminates 
 * generic firmware vulnerabilities such as race conditions during initialization, 
 * unhandled allocation failures, and orphan tasks causing heap fragmentation.
 * 
 * @warning In the future, the documentation will be restored to its normal
 *          form.
 * 
 * @note Before calling any function in this group, `eif_core_initialize()` 
 *       must be executed exactly once.<br><br>
 *       Examples: 
 *       @code{c}
 *       #include "esp_err.h"
 *       #include "esp_iot_framework_core.h"
 *       
 *       void app_main(void) {
 *           ESP_ERROR_CHECK(eif_core_initialize());
 *           
 *           // Further code...
 *       }
 *       @endcode
 */

/**
 * @brief Common helper for secure FreeRTOS task spawning.
 * 
 * This function provides a standardized mechanism to initialize FreeRTOS tasks.
 * It sequential-validates input arguments, ensures protection against double
 * spawning via the task handle state, and handles memory exhaustion failures 
 * safely.
 * 
 * @warning The `p_handle` argument must point to a persistent variable 
 *          (typically a module-scoped static) to maintain double-spawn 
 *          protection. Once the task finishes execution or triggers a delete,
 *          the underlying task handle variable must be set back to `NULL` to
 *          allow future respawns. 
 * 
 * @param p_handle  Pointer to the `TaskHandle_t` variable. If the pointed
 *                  handle is already initialized (not `NULL`), the function
 *                  returns `ESP_ERR_INVALID_STATE`.
 * @param f_worker  Task function (worker).
 * @param p_name    Task name string for debugging and identification.
 * @param u32_stack Stack size in bytes.
 * @param u_prio    Task priority.
 * 
 * @return 
 *    - `ESP_OK`:                Task created successfully.
 *    - `ESP_ERR_INVALID_ARG`:   The pointer `p_handle`, `f_worker` or `p_name`
 *                               is `NULL`.
 *    - `ESP_ERR_INVALID_STATE`: Task handle is already in use (task already 
 *                               exists).
 *    - `ESP_ERR_NO_MEM`:        Memory could not be allocated due to the lack
 *                               of an empty block of the required size.
 * 
 * Example of use (but it's better to use @ref EIF_TASK_LAUNCH macro):
 * @code{c}
 * #include "esp_log.h"
 * #include "freertos/FreeRTOS.h"
 * #include "freertos/task.h"
 * #include "esp_iot_framework_core_ext.h"
 * #include "esp_iot_framework_core_macros.h"
 * 
 * // Double-call protection
 * static TaskHandle_t mqtt_hdl = NULL;
 * 
 * void mqtt_task_worker(void *arg) {
 *     while(1) { 
 *         vTaskDelay(pdMS_TO_TICKS(1000));
 *     }
 *     mqtt_hdl = NULL;
 *     vTaskDelete(NULL);
 * }
 * 
 * void start_app_tasks(void) {
 *     EIF_TAG_WITH_UNUSED "APP_START";
 *     esp_err_t ret = ESP_OK;
 *     
 *     ret = eif_task_common_spawn(mqtt_hdl, mqtt_task_worker, "mqtt_service", 4096, 5);
 *     if (ret != ESP_OK) {
 *         EIF_LOG_E("Failed to launch MQTT task");
 *     }
 * }
 * @endcode
 * 
 * @note If double-spawn restrictions are not required (e.g., creating multiple
 *       instances of the same task), pass a pointer to a temporary standalone
 *       task handle variable instead of the main module handle.
 * 
 * Example of use (without double-call protection, but it's better to use 
 * @ref EIF_TASK_LAUNCH macro):
 * @code{c}
 * #include "esp_log.h"
 * #include "freertos/FreeRTOS.h"
 * #include "freertos/task.h"
 * #include "esp_iot_framework_core_ext.h"
 * #include "esp_iot_framework_core_macros.h"
 * 
 * void task_worker(void *arg) {
 *     while(1) { 
 *         vTaskDelay(pdMS_TO_TICKS(1000));
 *     }
 *     vTaskDelete(NULL);
 * }
 * 
 * void start_app_tasks(void) {
 *     EIF_TAG_WITH_UNUSED "APP_START";
 *     esp_err_t ret = ESP_OK;
 *     
 *     for (int i = 0; i < 5; i++) {
 *         TaskHandle_t hdl = NULL;
 *         
 *         ret = eif_task_common_spawn(hdl, task_worker, "task_worker", 2048, 3);
 *         if (ret != ESP_OK) {
 *             EIF_LOG_E("Failed to launch task iteration %d", i);
 *         }
 *     }
 * }
 * @endcode
 */
esp_err_t eif_task_common_spawn(
    TaskHandle_t * const p_handle, const TaskFunction_t f_worker,
    const char * const p_name, const uint32_t u32_stack,
    const UBaseType_t u_prio
);
/**
 * @brief Spawns an asynchronous task to execute a system reboot.
 * 
 * Spawns an asynchronous task to execute a system reboot.
 * 
 * @return 
 *    - `ESP_OK`:                Task created successfully and added to the
 *                               FreeRTOS scheduler.
 *    - `ESP_ERR_INVALID_ARG`:   Internal error. One of the mandatory arguments
 *                               is passed as a `NULL` pointer.
 *    - `ESP_ERR_INVALID_STATE`: The task handle is already active, meaning
 *                               this task is already running.
 *    - `ESP_ERR_NO_MEM`:        Memory could not be allocated due to the lack
 *                               of an empty block of the required size.
 */
esp_err_t eif_task_reboot_launch(void);
/**
 * @brief Spawns an asynchronous task to execute a Wi-Fi profile test.
 * 
 * Spawns an asynchronous task to execute a Wi-Fi profile test.
 * 
 * @param profile_index The index of the Wi-Fi profile under test. Must be
 *       from `0` to number of @ref wifi_profiles_desc "Wi-Fi profiles" stored
 *       in the system. It is set using `eif_set_wifi_profiles_count()`.
 * 
 * @return 
 *    - `ESP_OK`:                Task created successfully and added to the
 *                               FreeRTOS scheduler.
 *    - `ESP_ERR_INVALID_ARG`:   Internal error. One of the mandatory arguments
 *                               is passed as a `NULL` pointer.
 *    - `ESP_ERR_INVALID_STATE`: The task handle is already active, meaning
 *                               this task is already running.
 *    - `ESP_ERR_NO_MEM`:        Memory could not be allocated due to the lack
 *                               of an empty block of the required size.
 */
esp_err_t eif_task_wifi_test_launch(uint8_t profile_index);
#if (defined(CONFIG_EIF_ENABLE_TLS) || defined(DOXYGEN))
    /**
     * @brief Spawns an asynchronous task to regenerate TLS credentials.
     * 
     * Spawns an asynchronous task to regenerate TLS credentials.
     * 
     * @return 
     *    - `ESP_OK`:                Task created successfully and added to the
     *                               FreeRTOS scheduler.
     *    - `ESP_ERR_INVALID_ARG`:   Internal error. One of the mandatory
     *                               arguments is passed as a `NULL` pointer.
     *    - `ESP_ERR_INVALID_STATE`: The task handle is already active, meaning
     *                               this task is already running.
     *    - `ESP_ERR_NO_MEM`:        Memory could not be allocated due to the
     *                               lack of an empty block of the required size.
     */
    esp_err_t eif_task_tls_recreate_launch(void);
#endif
/**
 * @brief Spawns an asynchronous task to execute a firmware rollback.
 * 
 * Spawns an asynchronous task to execute a firmware rollback.
 * 
 * @return 
 *    - `ESP_OK`:                Task created successfully and added to the
 *                               FreeRTOS scheduler.
 *    - `ESP_ERR_INVALID_ARG`:   Internal error. One of the mandatory arguments
 *                               is passed as a `NULL` pointer.
 *    - `ESP_ERR_INVALID_STATE`: The task handle is already active, meaning
 *                               this task is already running.
 *    - `ESP_ERR_NO_MEM`:        Memory could not be allocated due to the lack
 *                               of an empty block of the required size.
 */
esp_err_t eif_task_rollback_and_reboot_launch(void);

/** @} */
/** @} */

#ifdef __cplusplus
}
#endif
#endif