/*
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp_iot_framework
 * Folder: include
 * File: esp_iot_framework.h
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

#ifndef ESP_IOT_FRAMEWORK_H
#define ESP_IOT_FRAMEWORK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "esp_err.h"
#include <stdbool.h>
#include "esp_wifi.h"
#include "sdkconfig.h"

/**
 * @defgroup config_c Framework Entry
 * @brief Base initialization and framework-wide settings.
 * 
 * This module contains functions that must be called at the very beginning 
 * of the application to prepare internal data structures and system tasks.
 * @{
 */

/**
 * @brief Initialize the framework and its core resources.
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
 * Example of use:
 * @code{c}
 * #include "esp_iot_framework.h"
 * 
 * void app_main(void) {
 *     eif_initialize();
 * 
 *     // Further code...
 * }
 * @endcode
 */
void eif_initialize(void);

/**
 * @brief Initialize Non-Volatile Storage (NVS) and framework data.
 * 
 * This function performs the following essential steps:
 * - Initializes the NVS flash partition (with automatic repair/erase if corrupted).
 * - Prepares or loads Wi-Fi profiles based on the configured count.
 * - Manages TLS credentials and authentication data (if enabled).
 * 
 * @note This function must be called after `eif_initialize()` but before 
 *       @ref eif_wifi_initialize(). This function is 
 *       **mandatory** for the framework to work. If you use 
 *       @ref eif_set_wifi_profiles_count(), it must be called before calling 
 *       @ref eif_nvs_initialize(). Otherwise, the default number of profiles (3) 
 *       will be loaded into NVS.
 * 
 * @warning If the NVS partition is corrupted or has a new version, this function 
 *          will automatically erase it and re-initialize, which results in the 
 *          loss of previously stored data.
 * 
 * @return 
 *    - `ESP_OK`: NVS and all framework fields initialized successfully.
 *    - `ESP_ERR_*`: Various NVS-related errors if hardware initialization fails.
 *                    Look at the logs to understand the cause of the errors.
 * 
 * Example of use:
 * @code{c}
 * #include "esp_err.h"
 * #include "esp_iot_framework.h"
 * 
 * void app_main(void) {
 *     eif_initialize();
 *     ESP_ERROR_CHECK(eif_nvs_initialize());
 * 
 *     // Further code...
 * }
 * @endcode
 */
esp_err_t eif_nvs_initialize(void);

/**
 * @brief Prototype for the pre-reboot user callback function.
 */
typedef void (*eif_pre_reboot_callback_t)(void);
/**
 * @brief Register a user-defined callback for the reboot sequence.
 * 
 * The registered function is executed during the system teardown, after 
 * the Wi-Fi and Web Server have been stopped, but before the actual restart.
 * 
 * @note By the time the `callback` is executed, the server and Wi-Fi module will 
 *       be already completely disabled.
 * 
 * @warning The callback is executed within a dedicated reboot task with a 
 *          limited stack size (`1024 bytes`). Avoid deep recursion, large local 
 *          arrays, or complex logic (like heavy logging or network operations) 
 *          inside the callback to prevent stack overflow.
 * 
 * @param callback Pointer to the function to execute. Cannot be NULL.
 * 
 * @return 
 *    - `ESP_OK`: Callback registered successfully.
 *    - `ESP_ERR_INVALID_ARG`: If `callback` is NULL.
 * 
 * Example of use:
 * @code{c}
 * #include "esp_iot_framework.h"
 * 
 * void my_callback(void) {
 *     // A good place to save data, set a relay to a safe position, or something else.
 * }
 * 
 * void app_main(void) {
 *     eif_initialize();
 *     eif_register_pre_reboot_callback(my_callback);
 *     
 *     // Further code...
 * }
 * @endcode
 */
esp_err_t eif_register_pre_reboot_callback(eif_pre_reboot_callback_t callback);
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
 * @brief Maximum number of custom Wi-Fi profiles.
 */
#define WIFI_PROFILES_MAX_COUNT 7

/**
 * @brief Configure Wi-Fi driver and power management policies.
 * 
 * Overrides the default low-level driver settings. These parameters are 
 * applied globally and define the performance and energy profile of the device.
 * 
 * @note This function must be called after @ref eif_initialize() but before 
 *       @ref eif_wifi_initialize().
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
 *    - `ESP_OK`:Settings saved successfully.
 *    - `ESP_ERR_INVALID_ARG`: If `wifi_driver_config` is NULL.
 * 
 * Example of use:
 * @code{c}
 * #include "esp_err.h"
 * #include "esp_wifi.h"
 * #include "esp_iot_framework.h"
 * 
 * void app_main(void) {
 *     eif_initialize();
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
 * @note This function must be called after @ref eif_initialize() but before 
 *       @ref eif_nvs_initialize().
 * 
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
 *       cannot be changed.
 * 
 * @param wifi_profiles_count Number of extra slots (0 to @ref WIFI_PROFILES_MAX_COUNT).
 * 
 * @return 
 *    - `ESP_OK`: Profile capacity set and state reset.
 *    - `ESP_ERR_INVALID_ARG`: If `count` exceeds @ref WIFI_PROFILES_MAX_COUNT.
 * 
 * 
 * Example of use:
 * @code{c}
 * #include "esp_err.h"
 * #include "esp_iot_framework.h"
 * 
 * void app_main(void) {
 *     eif_initialize();
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
 *    - `ESP_OK`: Subsystem initialized successfully.
 *    - `ESP_ERR_NO_MEM`: Failed to allocate memory for network interface.
 *    - `ESP_ERR_*`: Internal driver errors. Look at the logs to understand the 
 *                    cause of the errors.
 * 
 * Example of use:
 * @code{c}
 * #include "esp_err.h"
 * #include "esp_iot_framework.h"
 * 
 * void app_main(void) {
 *     eif_initialize();
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


/**
 * @defgroup eif_server HTTP(S) Server Configuration
 * @brief Management of HTTP/HTTPS endpoints and server tuning.
 * 
 * This module allows registering custom routes and fine-tuning server. The 
 * framework automatically manages the server lifecycle, starting it only when
 * a valid IP is obtained and stopping it upon disconnection.
 * 
 * @note To enable or disable encryption (TLS/HTTPS), use the 
 *       `CONFIG_EIF_ENABLE_TLS` flag from Kconfig via `idf.py menuconfig`.
 *       Depending on this setting, the server will operate in either HTTP 
 *       or HTTPS mode, and the corresponding configuration functions will 
 *       be available.
 * 
 * If you frequently intersect between HTTP and HTTPS during development and need 
 * custom server settings, you can use the following construct:
 * @code{c}
 * #include "esp_err.h"
 * #include "esp_https_server.h"
 * #include "esp_iot_framework.h"
 * 
 * void app_main(void) {
 *     eif_initialize();
 *     ESP_ERROR_CHECK(eif_nvs_initialize());
 * 
 *     #ifdef CONFIG_EIF_ENABLE_TLS
 *         // Creating default server settings
 *         httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();
 *         // Set custom settings unique to this type.
 *         config.httpd.server_port = 443;
 *         config.port_secure = 443;
 *         config.httpd.stack_size = 10240;
 *         // Selecting general settings
 *         httpd_config_t *cfg_httpd = &config.httpd;
 *     #else
 *         // Creating default server settings
 *         httpd_config_t config = HTTPD_DEFAULT_CONFIG();
 *         // Set custom settings unique to this type.
 *         config.server_port = 80;
 *         config.stack_size = 4096;
 *         // Selecting general settings
 *         httpd_config_t *cfg_httpd = &config;
 *     #endif
 * 
 *     // Changing general settings
 *     cfg_httpd->max_open_sockets = 4;
 *     cfg_httpd->recv_wait_timeout = 8;
 *     cfg_httpd->send_wait_timeout = 8;
 *     cfg_httpd->lru_purge_enable = true;
 * 
 *     #ifdef CONFIG_EIF_ENABLE_TLS
 *         ESP_ERROR_CHECK(eif_set_server_config_https(&config));
 *     #else
 *         ESP_ERROR_CHECK(eif_set_server_config_http(&config));
 *     #endif
 * 
 *     // Further code...
 * }
 * @endcode
 * @{
 */

#if defined(CONFIG_EIF_ENABLE_TLS) || defined(DOXYGEN)
    #include "esp_https_server.h"
    /**
     * @brief Tune the HTTPS server settings.
     * 
     * @note Only available if the Kconfig option `CONFIG_EIF_ENABLE_TLS` is enabled.
     * 
     * Overrides default HTTPS server parameters (ports, stack size, timeouts, 
     * etc.). You can also change the server's HTTP settings via `server_config`.
     * The framework creates an internal copy of the structure, so the original 
     * data's lifecycle no longer matters after the call. 
     * 
     * @note This function must be called after @ref eif_initialize() but before 
     *       @ref eif_wifi_initialize().
     * 
     * @warning The framework handles TLS credentials internally. To ensure 
     *          stability and prevent memory conflicts, the following fields in 
     *          `server_config` are explicitly overwritten with the following 
     *          values:
     *          - `cacert_pem` - `NULL`
     *          - `prvtkey_pem` - `NULL`
     *          - `cacert_len` - `0`
     *          - `prvtkey_len` - `0`
     *          - `transport_mode` - `HTTPD_SSL_TRANSPORT_SECURE`
     *          - `session_tickets` - `false`
     *          - `httpd.max_uri_handlers` - calculated automatically
     * 
     * @param server_config Pointer to server configuration. Use 
     *                      `HTTPD_SSL_CONFIG_DEFAULT()` as base.
     * @return 
     *    - `ESP_OK`: Configuration applied.
     *    - `ESP_ERR_INVALID_ARG`: If `server_config` is NULL.
     * 
     * Example of use:
     * @code{c}
     * #include "esp_err.h"
     * #include "esp_https_server.h"
     * #include "esp_iot_framework.h"
     * 
     * void app_main(void) {
     *     eif_initialize();
     *     ESP_ERROR_CHECK(eif_nvs_initialize());
     * 
     *     // Creating default server settings
     *     httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();
     *     // Changing server settings
     *     config.httpd.stack_size = 10240;
     *     config.httpd.server_port = 443;
     *     config.port_secure = 443;
     * 
     *     ESP_ERROR_CHECK(eif_set_server_config_https(&config));
     * 
     *     // Further code...
     * }
     * @endcode
     */
    esp_err_t eif_set_server_config_https(const httpd_ssl_config_t *server_config);
#endif
#if !defined(CONFIG_EIF_ENABLE_TLS) || defined(DOXYGEN)
    #include "esp_http_server.h"
    /**
     * @brief Tune the HTTP server settings.
     * 
     * @note Only available if the Kconfig option `CONFIG_EIF_ENABLE_TLS` is disabled.
     * 
     * Overrides default HTTP server parameters (ports, stack size, priority, 
     * timeouts, etc.). You can also change the server's HTTP settings via 
     * `server_config`. The framework creates an internal copy of the structure, 
     * so the original data's lifecycle no longer matters after the call.
     * 
     * @note This function must be called after @ref eif_initialize() but before 
     *       @ref eif_wifi_initialize().
     * 
     * @warning To ensure stability and prevent memory conflicts, the following 
     *          fields in `server_config` are explicitly overwritten with the 
     *          following values:
     *          - `max_uri_handlers` - calculated automatically
     * 
     * @param server_config Pointer to server configuration. Use 
     *                      `HTTPD_DEFAULT_CONFIG()` as base.
     * @return 
     *    - `ESP_OK`: Configuration applied.
     *    - `ESP_ERR_INVALID_ARG`: If `server_config` is NULL.
     * 
     * Example of use:
     * @code{c}
     * #include "esp_err.h"
     * #include "esp_http_server.h"
     * #include "esp_iot_framework.h"
     * 
     * void app_main(void) {
     *     eif_initialize();
     *     ESP_ERROR_CHECK(eif_nvs_initialize());
     * 
     *     // Creating default server settings
     *     httpd_config_t config = HTTPD_DEFAULT_CONFIG();
     *     // Changing server settings
     *     config.max_open_sockets = 3;
     *     config.recv_wait_timeout = 15;
     *     config.send_wait_timeout = 15;
     *     config.server_port = 80;
     * 
     *     ESP_ERROR_CHECK(eif_set_server_config_http(&config));
     * 
     *     // Further code...
     * }
     * @endcode
     */
    esp_err_t eif_set_server_config_http(const httpd_config_t *server_config);
#endif

/**
 * @brief Register custom URI handlers (endpoints).
 * 
 * @note This function must be called after @ref eif_initialize() but before 
 *       @ref eif_wifi_initialize().
 * 
 * Passes application-specific routes (e.g., `/api/data`, `/status`) to the 
 * framework. The passed handlers are automatically registered in the server 
 * at startup. The framework creates an internal copy of the structure, so the 
 * original data's lifecycle no longer matters after the call. When the function
 * is called again, the handlers saved in advance will be deleted.
 * 
 * @warning The handler array length must be equal to `uri_handlers_count`. If 
 *          the count exceeds the actual array length, a `Buffer Overread` will
 *          occur. If the count is less than the actual length, `Truncation`
 *          will occur (only the first `uri_handlers_count` elements will be 
 *          registered).
 *          <br>
 *          If the array is accessible by value at the function call site, you 
 *          can use the following code to automatically calculate the length: 
 *          @code{c}
 *          sizeof(array) / sizeof(array[0])
 *          @endcode
 * 
 * @param uri_handlers Array of URI structures defining paths and callbacks.
 * @param uri_handlers_count Number of elements in the array.
 * 
 * @return 
 *    - `ESP_OK`: Handlers stored successfully.
 *    - `ESP_ERR_INVALID_ARG`: If `uri_handlers` is NULL.
 *    - `ESP_ERR_NO_MEM`: Failed to allocate memory for the internal copy.
 * 
 * Example of use:
 * @code{c}
 * #include "esp_err.h"
 * #include "esp_log.h"
 * #include "esp_http_server.h"
 * #include "esp_iot_framework.h"
 * 
 * esp_err_t get_handler(httpd_req_t *req) {
 *     const char *resp = "Hello from ESP IoT Framework!";
 *     httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
 *     return ESP_OK;
 * }
 * 
 * esp_err_t post_handler(httpd_req_t *req) {
 *     ESP_LOGI("post_handler", "Data received!");
 *     httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
 *     return ESP_OK;
 * }
 * 
 * static const httpd_uri_t my_uris[] = {
 *     { .uri = "/hello", .method = HTTP_GET,  .handler = get_handler },
 *     { .uri = "/data",  .method = HTTP_POST, .handler = post_handler }
 * };
 * 
 * void app_main(void) {
 *     eif_initialize();
 *     ESP_ERROR_CHECK(eif_nvs_initialize());
 *     ESP_ERROR_CHECK(eif_set_uri_handlers(my_uris, 2));
 * 
 *     // Further code...
 * }
 * @endcode
 */
esp_err_t eif_set_uri_handlers(
    const httpd_uri_t *uri_handlers, size_t uri_handlers_count
);

/** @} */

#if defined(CONFIG_EIF_ENABLE_MDNS) || defined(DOXYGEN)
    #include "mdns.h"
    /**
     * @defgroup eif_mdns mDNS Configuration
     * @brief Network discovery and mDNS management
     * 
     * @note Only available if the Kconfig option `CONFIG_EIF_ENABLE_MDNS` is enabled.
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
     *       common hostname. To disable mDNS, turn off Kconfig option 
     *       `CONFIG_EIF_ENABLE_MDNS`.
     * 
     * If you frequently need to enable and disable mDNS during development, 
     * you can use the following construct:
     * @code{c}
     * #include "mdns.h"
     * #include "esp_err.h"
     * #include "esp_iot_framework.h"
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
     *     eif_initialize();
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
    #define MDNS_TXT_RECORDS_MAX_COUNT 32

    /**
     * @brief Configure the mDNS hostname and instance name.
     * 
     * Sets the base identity of the device on the local network. 
     * 
     * @note This function must be called after @ref eif_initialize() but before 
     *       @ref eif_wifi_initialize().
     * 
     * @param mdns_hostname Base name prefix (e.g., "my-sensor"). Max length: 32.
     * @param mdns_instance_name Friendly name for discovery tools (e.g., 
     *        "Main Hall Sensor"). If NULL or empty, the the formatted 
     *        `mdns_hostname`(with MAC) will be used as the instance name by 
     *        default.
     * 
     * @return 
     *    - `ESP_OK`: Configuration stored.
     *    - `ESP_ERR_INVALID_ARG`: If arguments are NULL.
     *    - `ESP_ERR_INVALID_SIZE`: If arguments are exceed maximum length.
     * 
     * Example of use:
     * @code{c}
     * #include "esp_err.h"
     * #include "esp_iot_framework.h"
     * 
     * void app_main(void) {
     *     eif_initialize();
     *     ESP_ERROR_CHECK(eif_nvs_initialize());
     *     ESP_ERROR_CHECK(eif_set_mdns("my-esp", "Smart Controller"));
     * 
     *     // Further code...
     * }   
     * @endcode
     */
    esp_err_t eif_set_mdns(
        const char* mdns_hostname, const char* mdns_instance_name
    );

    /**
     * @brief Set custom TXT records for mDNS service discovery.
     * 
     * TXT records allow you to broadcast additional metadata about the device 
     * (e.g., firmware version, model, or status). The framework creates an 
     * internal copy of the structure, so the original data's lifecycle no 
     * longer matters after the call.
     * 
     * @note This function must be called after @ref eif_initialize() but before 
     *       @ref eif_wifi_initialize().
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
     * @param txt_records Array of mDNS TXT items.
     * @param txt_records_count Number of elements in the array.
     * 
     * @return 
     *    - `ESP_OK`: Records applied.
     *    - `ESP_ERR_INVALID_ARG`: If NULL key/value is detected.
     *    - `ESP_ERR_INVALID_SIZE`: If the array length is less than `txt_records_count`
     * 
     * Example of use:
     * @code{c}
     * 
     * #include "mdns.h"
     * #include "esp_err.h"
     * #include "esp_iot_framework.h"
     * 
     * static const mdns_txt_item_t my_txt_records[] = {
     *     {"friendly_name", "Kitchen Main Light"},
     *     {"room",          "Kitchen"},
     *     {"hardware",      "ESP32-S3-DevKit"},
     *     {"v",             "1.0.4"}
     * };
     * 
     * void app_main(void) {
     *     eif_initialize();
     *     ESP_ERROR_CHECK(eif_nvs_initialize());
     *     ESP_ERROR_CHECK(eif_set_mdns_records(my_txt_records, 4));
     * 
     *     // Further code...
     * }
     * @endcode
     */
    esp_err_t eif_set_mdns_records(
        const mdns_txt_item_t txt_records[MDNS_TXT_RECORDS_MAX_COUNT], 
        size_t txt_records_count
    );
#endif

/** @} */


#ifdef __cplusplus
}
#endif
#endif