/*
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Folder: ./components/esp_iot_framework_device/include
 * File: esp_iot_framework_device.h
 * Library: esp_iot_framework_device
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

#ifndef ESP_IOT_FRAMEWORK_DEVICE_H
#define ESP_IOT_FRAMEWORK_DEVICE_H

#include "sdkconfig.h"

#include <stdint.h>
#include <esp_err.h>
#include <stdbool.h>
#include <esp_wifi.h>

#if (defined(CONFIG_EIF_ENABLE_TLS) || defined(DOXYGEN))
    #include <esp_https_server.h>
#endif
#if (!defined(CONFIG_EIF_ENABLE_TLS) || defined(DOXYGEN))
    #include <esp_http_server.h>
#endif

#ifdef __cplusplus
    extern "C" {
#endif

/**
 * @defgroup device_root Node: DEVICE
 * @copydoc md_docs_html_README_DEVICE
 * @{
 */
/**
 * @defgroup device_group DEVICE
 * @brief Public API for application development. Provides framework initialization, Wi-Fi configuration, network profile management, and system lifecycle hooks.
 */
/**
 * @defgroup device_rest_api REST API
 * @brief DEVICE REST API documentation.
 * @details @copydoc md_docs_html_REST_API_DEVICE
 */ 
/**
 * @defgroup device_kconfig Kconfig
 * @brief Node DEVICE `Kconfig` configuration options.
 * @copydoc md_docs_html_KCONFIG_DEVICE
 */
/** @} */

/**
 * @addtogroup device_group DEVICE
 * @{
 *
 * @details @note This group of modules is available when you include this line
 * at the beginning of the file.:
 * @code{c}
 * #include <esp_iot_framework_device.h>
 * @endcode
 */

/**
 * @defgroup device_c Device Boot
 * @brief Device startup and runtime environment initialization.
 *
 * This module initializes the device hardware and prepares the runtime
 * environment for the application layer.
 * @{
 */


/**
 * @brief Configures system-wide services and prepares the device networking environment.
 *
 * Sets up the base runtime environment, applies initial device configurations,
 * and binds essential system handlers required to start the device.
 *
 * @note This function should be called immediately after `eif_core_initialize()`.
 *
 * @return
 *    - `ESP_OK`: Device layer initialization was successful.
 *    - `ESP_ERR_INVALID_ARG`: Internal error. One of the mandatory arguments
 *                             is passed as a `NULL` pointer.
 *
 * Example of use:
 * @code{c}
 * #include <esp_err.h>
 * #include <esp_iot_framework_core.h>
 * #include <esp_iot_framework_device.h>
 *
 * void app_main(void) {
 *     ESP_ERROR_CHECK(eif_core_initialize());
 *     ESP_ERROR_CHECK(eif_device_initialize());
 *
 *     // Further code...
 * }
 * @endcode
 */
esp_err_t eif_device_initialize(void);

/** @} */



/**
 * @defgroup eif_server HTTP(S) Server
 * @brief Management of HTTP/HTTPS endpoints and server tuning.
 *
 * This module allows registering custom routes and fine-tuning server. The
 * framework automatically manages the server lifecycle, starting it only when
 * a valid IP is obtained and stopping it upon disconnection.
 *
 * @note To enable or disable encryption (TLS/HTTPS), use the <code>
 *         <a href="group__core__kconfig.html#CONFIG_EIF_ENABLE_TLS">
 *           CONFIG_EIF_ENABLE_TLS
 *         </a>
 *       </code> flag from `Kconfig` via `idf.py menuconfig`.
 *       Depending on this setting, the server will operate in either HTTP
 *       or HTTPS mode, and the corresponding configuration functions will
 *       be available.
 *
 * If you frequently intersect between HTTP and HTTPS during development and need
 * custom server settings, you can use the following construct:
 * @code{c}
 * #include <esp_err.h>
 * #include <esp_https_server.h>
 * #include <esp_iot_framework_core.h>
 * #include <esp_iot_framework_device.h>
 *
 * void app_main(void) {
 *     ESP_ERROR_CHECK(eif_core_initialize());
 *     ESP_ERROR_CHECK(eif_device_initialize());
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

#if (defined(CONFIG_EIF_ENABLE_TLS) || defined(DOXYGEN))
    /**
     * @brief Tune the HTTPS server settings.
     *
     * @note Only available if the `Kconfig` option <code>
     *         <a href="group__core__kconfig.html#CONFIG_EIF_ENABLE_TLS">
     *           CONFIG_EIF_ENABLE_TLS
     *         </a>
     *       </code> is enabled.
     *
     * Overrides default HTTPS server parameters (ports, stack size, timeouts,
     * etc.). You can also change the server's HTTP settings via `server_config`.
     * The framework creates an internal copy of the structure, so the original
     * data's lifecycle no longer matters after the call.
     *
     * @note This function must be called after `eif_device_initialize()` but before
     *       `eif_wifi_initialize()`.
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
     *    - `ESP_ERR_INVALID_ARG`: If `server_config` is `NULL`.
     *
     * Example of use:
     * @code{c}
     * #include <esp_err.h>
     * #include <esp_https_server.h>
     * #include <esp_iot_framework_core.h>
     * #include <esp_iot_framework_device.h>
     *
     * void app_main(void) {
     *     ESP_ERROR_CHECK(eif_core_initialize());
     *     ESP_ERROR_CHECK(eif_device_initialize());
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
    esp_err_t eif_set_server_config_https(
        const httpd_ssl_config_t * const server_config
    );
#endif
#if (!defined(CONFIG_EIF_ENABLE_TLS) || defined(DOXYGEN))
    /**
     * @brief Tune the HTTP server settings.
     *
     * @note Only available if the `Kconfig` option <code>
     *         <a href="group__core__kconfig.html#CONFIG_EIF_ENABLE_TLS">
     *           CONFIG_EIF_ENABLE_TLS
     *         </a>
     *       </code> is disabled.
     *
     * Overrides default HTTP server parameters (ports, stack size, priority,
     * timeouts, etc.). You can also change the server's HTTP settings via
     * `server_config`. The framework creates an internal copy of the structure,
     * so the original data's lifecycle no longer matters after the call.
     *
     * @note This function must be called after `eif_device_initialize()` but before
     *       `eif_wifi_initialize()`.
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
     *    - `ESP_ERR_INVALID_ARG`: If `server_config` is `NULL`.
     *
     * Example of use:
     * @code{c}
     * #include <esp_err.h>
     * #include <esp_http_server.h>
     * #include <esp_iot_framework_core.h>
     * #include <esp_iot_framework_device.h>
     *
     * void app_main(void) {
     *     ESP_ERROR_CHECK(eif_core_initialize());
     *     ESP_ERROR_CHECK(eif_device_initialize());
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
    esp_err_t eif_set_server_config_http(
        const httpd_config_t * const server_config
    );
#endif

/**
 * @brief Register custom URI handlers (endpoints).
 *
 * @note This function must be called after `eif_device_initialize()` but before
 *       `eif_wifi_initialize()`.
 *
 * Passes application-specific routes (e.g., `/api/data`, `/status`) to the
 * framework. The passed handlers are automatically registered in the server
 * at startup. The framework creates an internal copy of the structure, so the
 * original data's lifecycle no longer matters after the call. When the function
 * is called again, the handlers saved in advance will be deleted.
 *
 * @warning The handler array length must be equal to `uri_handlers_count`. If
 *          the count exceeds the actual array length, a `Load Prohibited` will
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
 *    - `ESP_ERR_INVALID_ARG`: If `uri_handlers` is `NULL`.
 *    - `ESP_ERR_NO_MEM`: Failed to allocate memory for the internal copy.
 *
 * Example of use:
 * @code{c}
 * #include <esp_err.h>
 * #include <esp_log.h>
 * #include <esp_http_server.h>
 * #include <esp_iot_framework_core.h>
 * #include <esp_iot_framework_device.h>
 *
 * esp_err_t get_handler(httpd_req_t *req) {
 *     const char *resp = "Hello from esp_iot_framework_device!";
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
 *     ESP_ERROR_CHECK(eif_core_initialize());
 *     ESP_ERROR_CHECK(eif_device_initialize());
 *     ESP_ERROR_CHECK(eif_nvs_initialize());
 *     ESP_ERROR_CHECK(eif_set_uri_handlers(my_uris, 2));
 *
 *     // Further code...
 * }
 * @endcode
 */
esp_err_t eif_set_uri_handlers(
    const httpd_uri_t * const uri_handlers, size_t uri_handlers_count
);

/** @} */
/** @} */

#ifdef __cplusplus
    }
#endif
#endif