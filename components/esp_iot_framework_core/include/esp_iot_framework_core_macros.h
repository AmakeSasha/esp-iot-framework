/* SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Library: esp_iot_framework_core
 * Folder: ./components/esp_iot_framework_core/include
 * File: esp_iot_framework_core_macros.h
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

#ifndef ESP_IOT_FRAMEWORK_CORE_MACROS_H
#define ESP_IOT_FRAMEWORK_CORE_MACROS_H

#include "sdkconfig.h"
#if (defined(CONFIG_EIF_ENABLE_TLS) || defined(DOXYGEN))
    #include <mbedtls/error.h>
#endif

#ifdef __cplusplus
    extern "C" {
#endif

/**
 * @addtogroup core_macros_group Core Macros
 * @{
 *
 * @details @note This group of modules is available when you include this line
 * at the beginning of the file.:
 * @code{c}
 * #include <esp_iot_framework_core_macros.h>
 * @endcode
 *
 * This file contains core macros used throughout the framework. They enforce
 * uniform error handling and simplify the development of new nodes.
 */

/**
 * @defgroup macros_log Logging
 * @brief Simple console logging macros with config level control.
 * @{
 *
 * This group contains macros for printing formatted messages to the console
 * output. The output style and threshold level are configured via `Kconfig`.
 */

/**
 * @brief Helper to define a local logging 'TAG' identifier.
 *
 * This macro declares a string named `TAG`. It automatically adds an unused
 * attribute (`[[maybe_unused]]` for C++, `__attribute__((unused))` for C or
 * nothing if the compiler does not support it) to suppress compiler warnings
 * and satisfy MISRA C requirements when all logging macros in the scope are
 * stripped out by the preprocessor.
 *
 * @note This macro is intended for use inside functions (within local scopes).
 *       It allows defining a local logging context within a specific scope,
 *       fully complying with the MISRA C rule that strictly forbids the use
 *       of `#undef`.
 *
 * Example of use:
 * @code{c}
 * #include <esp_log.h>
 * #include <esp_iot_framework_core_macros.h>
 *
 * void process_sensor_event(void) {
 *     EIF_TAG_WITH_UNUSED "SHT3X_DRV";
 *
 *     // Logic here...
 *
 *     EIF_LOG_D("Data processed");
 * }
 * @endcode
 */
#if defined(__cplusplus)
    #define EIF_TAG_WITH_UNUSED static const char* const TAG [[maybe_unused]] =
#elif (defined(__GNUC__) || defined(__clang__) || defined(DOXYGEN))
    #define EIF_TAG_WITH_UNUSED static const char* const TAG __attribute__((unused)) =
#else
    #define EIF_TAG_WITH_UNUSED static const char* const TAG =
#endif

/**
 * @brief Low-level macro to format and output messages to the ESP log system.
 *
 * This macro forwards messages directly to the ESP logging system. It implicitly
 * uses the local `TAG` variable, which must be defined in the current scope.
 *
 * @note This macro implicitly relies on a `TAG` identifier being defined and
 *       accessible within the current scope. Behavior depends on the
 *       `CONFIG_EIF_LOG_SHOW_METADATA` configuration:
 *       - **Enabled:** Automatically injects source metadata (file name, line
 *         number, and function name) at the beginning of the log message.
 *         Example of output to the console:
 *         @code{text}
 *         I (8536) HTTPS server: src/network.c:79 (mdns_initialize) mDNS started. Link: https://device-aabbcc.local
 *         @endcode
 *        
 *       - **Disabled:** Outputs the pure log message without any metadata.
 *         Example of output to the console:
 *         @code{text}
 *         I (8536) HTTPS server: mDNS started. Link: https://device-aabbcc.local
 *         @endcode
 *
 * @param[in] m_macro  Target ESP-IDF logging macro (e.g., `ESP_LOGI`, `ESP_LOGW`, `ESP_LOGE`).
 * @param[in] m_format Format string compatible with `printf`.
 * @param[in] ...      Optional variadic arguments for the format string.
 *
 * Example of use:
 * @code{c}
 * #include <esp_log.h>
 * #include <esp_system.h>
 * #include <esp_iot_framework_core_macros.h>
 *
 * #define TAG "SYS_MON"
 *
 * void monitor_heap_usage(void) {
 *     uint32_t free_heap = esp_get_free_heap_size();
 *     if (free_heap < 10 * 1024) {
 *         EIF_PRINT(ESP_LOGW, "Critical memory level: %u bytes remaining!", free_heap);
 *     }
 * }
 * @endcode
 */
/* @deviation [Rule 1.1] The GNU/Clang '##__VA_ARGS__' extension is used to
 * safely eliminate trailing commas in empty variadic macro invocations. This
 * extension is fully supported and deterministic within the target Espressif
 * toolchains, posing zero risk of translation limit exhaustion or syntax
 * corruption.
 * @deviation [Rule 20.10] The '##' operator is constrained to the
 * compiler-specific variadic comma-deletion extension. It does not perform
 * arbitrary token modification and strictly expands to well-defined,
 * predictable C code.
 * @deviation [Rule 20.11] Type safety for variadic arguments is strictly
 * enforced at compile-time via the underlying
 * '__attribute__((format(printf, ...)))' integrated into the ESP-IDF logging
 * subsystem, supplemented by peer code reviews. */
#if (defined(CONFIG_EIF_LOG_SHOW_METADATA) || defined(DOXYGEN))
    /* Allowed by the @deviation [Rule 1.1, 20.10, 20.11] specified above */
    #define EIF_PRINT(m_macro, m_format, ...) m_macro(TAG, "%s:%d (%s) " m_format, __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#endif
#if (!defined(CONFIG_EIF_LOG_SHOW_METADATA) && !defined(DOXYGEN))
    /* Allowed by the @deviation [Rule 1.1, 20.10, 20.11] specified above */
    #define EIF_PRINT(m_macro, m_format, ...) m_macro(TAG, m_format, ##__VA_ARGS__)
#endif

/**
 * @name Logging Severity Levels
 * @{
 */
/** @brief Error log level severity. */
#define EIF_LOG_LEVEL_E 1
/** @brief Warning log level severity. */
#define EIF_LOG_LEVEL_W 2
/** @brief Informational log level severity. */
#define EIF_LOG_LEVEL_I 3
/** @brief Debug log level severity. */
#define EIF_LOG_LEVEL_D 4
/** @} */

/**
 * @name Application Logging Interface
 * @{
 * @brief Main logging macros with global compile-time filtering.
 *
 * Use these macros to print logs to the console. Which logs actually
 * get compiled into the firmware depends on the `CONFIG_EIF_LOG_LEVEL` value.
 * A logging macro works only if `CONFIG_EIF_LOG_LEVEL` is equal to or higher
 * than its corresponding severity level constant (from 1 to 4).
 *
 * For example: if `CONFIG_EIF_LOG_LEVEL` is set to `EIF_LOG_LEVEL_W` (2), then
 * `EIF_LOG_E` and `EIF_LOG_W` will print logs normally. However, `EIF_LOG_I`
 * and `EIF_LOG_D` will be completely ignored.
 *
 * When a log level is ignored, its macro turns into an empty `((void)0U)`.
 * The compiler completely deletes these lines and their text strings.
 *
 * @note All macros in this interface implicitly rely on a `TAG` identifier
 *       being defined and accessible within the current execution scope.
 *
 * @param[in] ... Format string compliant with standard printf specifiers,
 *                followed by matching variadic arguments.
 *
 * Example of use:
 * @code{c}
 * #include <esp_log.h>
 * #include <esp_iot_framework_core_macros.h>
 *
 * void check_system_health(float voltage, bool is_connected) {
 *     EIF_TAG_WITH_UNUSED "HEALTH"; // Required for EIF_LOG_x
 *
 *     if (voltage < 3.2f) {
 *         EIF_LOG_E("Voltage critical: %.2fV", voltage);
 *     } else if (!is_connected) {
 *         EIF_LOG_W("Node offline, but voltage is OK (%.2fV)", voltage);
 *     } else {
 *         EIF_LOG_I("System healthy");
 *     }
 * }
 * @endcode
 */
/* All of the '@deviation' listed below apply to all macros prefixed with
 *'EIF_LOG_' declared in this file:
 * @deviation [Rule 20.11] These interface macros act as strict pass-through
 * wrappers to the underlying EIF_PRINT layout. Type safety for all variadic
 * arguments is structurally enforced at compile-time by the toolchain via
 * printf compiler attributes, completely eliminating runtime type mismatch
 * risks. */
/** @brief Logs an error message using the ESP_LOGE severity. */
#if (CONFIG_EIF_LOG_LEVEL >= EIF_LOG_LEVEL_E || defined(DOXYGEN))
    #define EIF_LOG_E(...) EIF_PRINT(ESP_LOGE, __VA_ARGS__)
#else
    #define EIF_LOG_E(...) ((void)0U)
#endif
/** @brief Logs a warning message using the ESP_LOGW severity. */
#if (CONFIG_EIF_LOG_LEVEL >= EIF_LOG_LEVEL_W || defined(DOXYGEN))
    #define EIF_LOG_W(...) EIF_PRINT(ESP_LOGW, __VA_ARGS__)
#else
    #define EIF_LOG_W(...) ((void)0U)
#endif
/** @brief Logs an informational message using the ESP_LOGI severity. */
#if (CONFIG_EIF_LOG_LEVEL >= EIF_LOG_LEVEL_I || defined(DOXYGEN))
    #define EIF_LOG_I(...) EIF_PRINT(ESP_LOGI, __VA_ARGS__)
#else
    #define EIF_LOG_I(...) ((void)0U)
#endif
/** @brief Logs a debug message using the ESP_LOGD severity. */
#if (CONFIG_EIF_LOG_LEVEL >= EIF_LOG_LEVEL_D || defined(DOXYGEN))
    #define EIF_LOG_D(...) EIF_PRINT(ESP_LOGD, __VA_ARGS__)
#else
    #define EIF_LOG_D(...) ((void)0U)
#endif
/** @} */

/**
 * @brief Unconditionally executes an expression and logs warning details on failure.
 *
 * This macro executes the expression `m_expr` regardless of any previous status.
 * If the resulting execution status is not `ESP_OK`, it fetches the error name
 * via `esp_err_to_name()` and outputs a warning-level log containing the
 * failure context. The execution status is always written to `m_result`. It is
 * ideal for cleanup blocks or logging forced actions, such as closing sockets
 * before restarting.
 *
 * @warning This macro runs unconditionally and will overwrite the previous
 *          value of `m_result`. Do not use it inside sequential checking chains
 *          (`EIF_IF_OK_*`) as it can mask earlier initialization failures.
 *
 * @param[out] m_result Status variable (`esp_err_t`) to store the execution
 *                      result. The previous value is overwritten
 *                      unconditionally.
 * @param[in]  m_expr   The function call or expression returning `esp_err_t`.
 * @param[in]  m_format Printf-compliant format string for warning details.
 * @param[in]  ...      Optional variadic arguments matching the format string.
 *
 * Example of use:
 * @code{c}
 * #include <esp_log.h>
 * #include <esp_wifi.h>
 * #include <nvs_flash.h>
 * #include <esp_iot_framework_core_macros.h>
 *
 * void system_reboot_prepare(void) {
 *     EIF_TAG_WITH_UNUSED "REBOOT";
 *     esp_err_t res = ESP_OK;
 *
 *     // We attempt to stop services regardless of 'res' value.
 *     // If a step fails, we get a warning, but the sequence continues.
 *     EIF_SHOW_ESP_ERR_T(res, esp_wifi_stop(), "Wi-Fi stop failed");
 *     EIF_SHOW_ESP_ERR_T(res, nvs_flash_deinit(), "NVS deinit failed");
 *
 *     esp_restart();
 * }
 * @endcode
 */
#if (CONFIG_EIF_LOG_LEVEL > EIF_LOG_LEVEL_E || defined(DOXYGEN))
    #define EIF_SHOW_ESP_ERR_T(m_result, m_expr, m_format, ...) \
do { \
    (m_result) = (m_expr); \
    if ((m_result) != (ESP_OK)) { \
        const char* l_err_name = esp_err_to_name((m_result)); \
        EIF_LOG_W("%s: " m_format, \
            ((l_err_name != NULL) ? l_err_name : "UNKNOWN_ERROR") \
            /* @deviation [Rule 20.10] The '##' token pasting operator is
             * utilized strictly for the compiler-specific comma deletion
             * extension. The expansion is completely deterministic and
             * guaranteed to yield valid, predictable syntax without risk of
             * expression corruption. */ \
            , ##__VA_ARGS__); \
    } \
} while (false)
#else
    #define EIF_SHOW_ESP_ERR_T(m_result, m_expr, m_format, ...) \
do { \
    (m_result) = (m_expr); \
} while (false)
#endif
/** @} */

/**
 * @defgroup macros_check Checkers
 * @brief Sequential error handling and parameter validation macros.
 * @{
 *
 * These macros execute code in chains using a status variable (`m_result`).
 * If a previous operation failed, subsequent steps are skipped, avoiding
 * deeply nested if-else statements while maintaining strict MISRA C compliance
 * (without `goto` and `return` in the middle of the function).
 */

/**
 * @brief Validates a pointer variable if the current execution state is successful.
 *
  * This macro performs a conditional validation check. It verifies whether the
 * provided pointer `m_ptr` is `NULL`, but only if `m_result` currently equals
 * `ESP_OK`. If the pointer is `NULL`, the status variable `m_result` is set to
 * `m_error`, and it logs a critical error containing the stringified name of
 * the failed parameter. If `m_result` already contains an error, the check is
 * skipped entirely.
 *
 * @param[in,out] m_result Variable holding the active sequence status
 *                         (`esp_err_t`).
 * @param[in]     m_ptr    The pointer identifier or reference to be checked
 *                         against `NULL`.
 * @param[in]     m_error  The custom error status code to assign to `m_result`
 *                          if validation fails (type `esp_err_t`).
 *
 * Example of use:
 * @code{c}
 * #include <esp_log.h>
 * #include <esp_wifi.h>
 * #include <esp_iot_framework_core_macros.h>
 *
 * esp_err_t wifi_quick_init(const wifi_config_t *cfg) {
 *     EIF_TAG_WITH_UNUSED "WIFI_INIT";
 *     esp_err_t ret = ESP_OK;
 *
 *     // If any macro fails, 'ret' changes and all subsequent macros are skipped.
 *     EIF_IF_OK_CHECK_NOT_NULL(ret, cfg, ESP_ERR_INVALID_ARG);
 *     EIF_IF_OK_CHECK_ESP_ERR_T(ret, esp_wifi_set_mode(WIFI_MODE_STA),
 *         "Set mode failed");
 *     EIF_IF_OK_CHECK_ESP_ERR_T(ret, esp_wifi_set_config(WIFI_IF_STA, (wifi_config_t*)cfg),
 *         "Set config failed");
 *     EIF_IF_OK_CHECK_ESP_ERR_T(ret, esp_wifi_start(),
 *         "Start failed");
 *
 *     return ret;
 * }
 * @endcode
 */
#if (CONFIG_EIF_LOG_LEVEL > EIF_LOG_LEVEL_E || defined(DOXYGEN))
    #define EIF_IF_OK_CHECK_NOT_NULL(m_result, m_ptr, m_error) \
do { \
    if ((m_result) == (ESP_OK)) { \
        if ((m_ptr) == NULL) { \
            /* @deviation [Rule 20.10] The '#' stringification operator is used
             * exclusively for diagnostic logging to evaluate the parameter
             * identifier. The expansion evaluates strictly to a well-formed,
             * immutable string literal, completely eliminating risks of side
             * effects or undefined behavior. */ \
            EIF_LOG_E("PARAMETER_IS_NULL: '%s'", #m_ptr); \
            (m_result) = (m_error); \
        } \
    } \
} while(false)
#else
    #define EIF_IF_OK_CHECK_NOT_NULL(m_result, m_ptr, m_error) \
do { \
    if ((m_result) == (ESP_OK)) { \
        if ((m_ptr) == NULL) { \
            (m_result) = (m_error); \
        } \
    } \
} while(false)
#endif

/**
 * @brief Executes and validates an ESP-IDF expression sequentially.
 *
 * If `m_result` equals `ESP_OK`, this macro executes `m_expr` and writes its
 * return status directly into `m_result`. On failure (result is not `ESP_OK`),
 * it extracts the error name via `esp_err_to_name()` and logs a critical error
 * prefixed with that name. If `m_result` is already an error state (is not
 * `ESP_OK`), `m_expr` is skipped entirely.
 *
 * @param[in,out] m_result Status variable (`esp_err_t`). Checked before
 *                         execution; overwritten with the return value of
 *                         `m_expr`.
 * @param[in]     m_expr   The function call or expression returning `esp_err_t`.
 * @param[in]     m_format Printf-compliant format string for custom error details.
 * @param[in]     ...      Optional variadic format arguments matching the
 *                         format string.
 *
 * Example of use:
 * @code{c}
 * #include <esp_log.h>
 * #include <esp_wifi.h>
 * #include <esp_iot_framework_core_macros.h>
 *
 * esp_err_t wifi_quick_init(const wifi_config_t *cfg) {
 *     EIF_TAG_WITH_UNUSED "WIFI_INIT";
 *     esp_err_t ret = ESP_OK;
 *
 *     // If any macro fails, 'ret' changes and all subsequent macros are skipped.
 *     EIF_IF_OK_CHECK_NOT_NULL(ret, cfg, ESP_ERR_INVALID_ARG);
 *     EIF_IF_OK_CHECK_ESP_ERR_T(ret, esp_wifi_set_mode(WIFI_MODE_STA),
 *         "Set mode failed");
 *     EIF_IF_OK_CHECK_ESP_ERR_T(ret, esp_wifi_set_config(WIFI_IF_STA, (wifi_config_t*)cfg),
 *         "Set config failed");
 *     EIF_IF_OK_CHECK_ESP_ERR_T(ret, esp_wifi_start(),
 *         "Start failed");
 *
 *     return ret;
 * }
 * @endcode
 */
#if (CONFIG_EIF_LOG_LEVEL > EIF_LOG_LEVEL_E || defined(DOXYGEN))
    #define EIF_IF_OK_CHECK_ESP_ERR_T(m_result, m_expr, m_format, ...) \
do { \
    if ((m_result) == (ESP_OK)) { \
        (m_result) = (m_expr); \
        if ((m_result) != (ESP_OK)) { \
            const char* l_err_name = esp_err_to_name((m_result)); \
            EIF_LOG_E("%s: " m_format, \
                ((l_err_name != NULL) ? l_err_name : "UNKNOWN_ERROR") \
                /* @deviation [Rule 20.10] The '##' token pasting operator is
                 * utilized strictly for the compiler-specific comma deletion
                 * extension. The expansion is completely deterministic and
                 * guaranteed to yield valid, predictable syntax without risk
                 * of expression corruption. */ \
                , ##__VA_ARGS__); \
        } \
    } \
} while(false)
#else
    #define EIF_IF_OK_CHECK_ESP_ERR_T(m_result, m_expr, m_format, ...) \
do { \
    if ((m_result) == (ESP_OK)) { \
        (m_result) = (m_expr); \
    } \
} while(false)
#endif

/**
 * @brief Checks a condition and sets an error status if the condition is true, only when the current status is OK.
 *
 * If `m_ret` equals `ESP_OK`, this macro evaluates `m_cond`. If the condition
 * evaluates to `true`, it logs a critical error using the provided message
 * (`m_info`) and sets `m_ret` to the specified error code (`m_err`). If `m_ret`
 * is already in an error state (not `ESP_OK`), the condition check and logging
 * are skipped entirely.
 *
 * This macro is useful for validating preconditions (e.g., null checks, range
 * checks) that must be satisfied before proceeding with further operations.
 *
 *
 * @param[in,out] m_ret    Status variable (`esp_err_t`). Checked before
 *                         evaluation; overwritten with `m_err` if `m_cond` is
 *                         true.
 * @param[in]     m_cond   The condition to evaluate. Must be an expression that
 *                         can be interpreted as boolean (`true`/`false`).
 * @param[in]     m_err    The error code (`esp_err_t`) to assign to `m_ret` if
 *                         `m_cond` is true.
 * @param[in]     m_format Printf-compliant format string for custom error details.
 * @param[in]     ...      Optional variadic format arguments matching the
 *                         format string.
 *
 * Example of use:
 * @code{c}
 * #include <esp_log.h>
 * #include <esp_iot_framework_core_macros.h>
 *
 * esp_err_t process_data(const char* data, size_t length) {
 *     EIF_TAG_WITH_UNUSED "PROCESS_DATA";
 *     esp_err_t ret = ESP_OK;
 *
 *     // Check for invalid arguments only if no previous error occurred
 *     EIF_IF_OK_CHECK_CONDITION(ret, data == NULL, ESP_ERR_INVALID_ARG,
 *         "Data pointer is NULL");
 *     EIF_IF_OK_CHECK_CONDITION(ret, length == 0, ESP_ERR_INVALID_SIZE,
 *         "Data length is zero");
 *
 *     if (ret == ESP_OK) {
 *         // Proceed with processing only if all checks passed
 *         ret = perform_actual_processing(data, length);
 *         if (ret != ESP_OK) {
 *             EIF_LOG_E("Processing failed: %s", esp_err_to_name(ret));
 *         }
 *     }
 *
 *     return ret;
 * }
 * @endcode
 */
#if (CONFIG_EIF_LOG_LEVEL > EIF_LOG_LEVEL_E || defined(DOXYGEN))
    #define EIF_IF_OK_CHECK_CONDITION(m_ret, m_cond, m_err, m_format, ...) \
do { \
    if ((m_ret) == ESP_OK) { \
        if ((m_cond)) { \
            /* @deviation [Rule 20.10] The '##' token pasting operator is
             * used here to delete the comma when '__VA_ARGS__' is empty.
             * This is a necessary compiler-specific extension for
             * variadic logging. Compilation is impossible without '##'. */ \
            EIF_LOG_E(m_format, ##__VA_ARGS__); \
            (m_ret) = (m_err); \
        } \
    } \
} while(false)
#else
    #define EIF_IF_OK_CHECK_CONDITION(m_ret, m_cond, m_err, m_format, ...) \
do { \
    if ((m_ret) == ESP_OK) { \
        if ((m_cond)) { \
            (m_ret) = (m_err); \
        } \
    } \
} while(false)
#endif

#if (defined(CONFIG_EIF_ENABLE_TLS) || defined(DOXYGEN))
    /**
     * @brief Executes and validates an MbedTLS expression sequentially.
     *
     * @note Only available if the Kconfig option `CONFIG_EIF_ENABLE_TLS` is enabled.
     *
     * This macro operates similarly to `EIF_IF_OK_CHECK_ESP_ERR_T`, but is
     * specifically designed for the MbedTLS library, which uses `int` (where
     * `0` is success and negative values are errors) instead of `esp_err_t`.
     *
     * If `m_ret` equals `0`, it executes `m_expr` and stores the result back in
     * `m_ret`. On failure, it converts the MbedTLS error code into a
     * human-readable string using `mbedtls_strerror()` and logs it along with
     * the absolute hex value and `func_name`.
     *
     * @note The variable passed to `m_ret` must be of type `int`, NOT
     *       `esp_err_t`. Additionally, human-readable error strings require
     *       `CONFIG_MBEDTLS_ERROR_C` to be enabled in the project configuration.
     *       Otherwise, `mbedtls_strerror` might output a generic or empty string.
     *
     * @param[in,out] m_ret     Status variable (`int`). Checked before execution;
     *                          overwritten with the return value of `m_expr`.
     * @param[in]     m_expr    The MbedTLS function call returning an `int`.
     * @param[in]     func_name String literal representing the operation name
     *                          (for logging).
     *
     * Example of use:
     * @code{c}
     * #include <esp_log.h>
     * #include <mbedtls/ssl.h>
     * #include <esp_iot_framework_core_macros.h>
     *
     * int open_secure_socket(mbedtls_ssl_context *ssl) {
     *     EIF_TAG_WITH_UNUSED "TLS_WRAP";
     *     int ret = 0; // MbedTLS uses int
     *
     *     EIF_IF_OK_CHECK_MBEDTLS_ERR(ret, mbedtls_ssl_session_reset(ssl), "Session reset");
     *     EIF_IF_OK_CHECK_MBEDTLS_ERR(ret, mbedtls_ssl_handshake(ssl), "Handshake");
     *
     *     return ret;
     * }
     * @endcode
     */
    #define EIF_IF_OK_CHECK_MBEDTLS_ERR(m_ret, m_expr, func_name) \
do { \
    if ((m_ret) == (0)) { \
        (m_ret) = (m_expr); \
        if ((m_ret) != 0) { \
            char err_buf[100] = {0}; \
            mbedtls_strerror((m_ret), err_buf, sizeof(err_buf)); \
            EIF_LOG_E("MbedTLS %s failed: %s (-0x%04x)", \
                (func_name), err_buf, -(m_ret)); \
        } \
    } \
} while(false)
#endif
/** @} */

/**
 * @defgroup macros_other Other
 * @brief Miscellaneous helper macros.
 * @{
 *
 * @details This group serves as a collection point for utility macros that
 * currently do not belong to any specific category. It includes tools that
 * are either unique in their purpose or haven't yet reached a sufficient
 * "critical mass" to justify the creation of a dedicated group.
 */

/**
 * @brief Spawns a system task using standardized, auto-resolved configuration macros.
 *
 * This macro simplifies task creation by enforcing a unified naming convention
 * for task parameters. By providing a base alias (`m_alias`), the macro
 * automatically constructs the necessary configuration definitions (Name, Stack
 * Size, Priority) using the `##` token pasting operator.
 *
 * For this macro to compile, you must define the corresponding parameters
 * before calling it. For an alias `X`, the following definitions must exist:
 * - `TASK_X_NAME`     (String literal, task name in the scheduler)
 * - `TASK_X_SIZE`     (Integer, stack size in words (4 byte))
 * - `TASK_X_PRIORITY` (Integer, FreeRTOS priority)
 *
 * @warning The `m_handle` argument must point to a persistent variable
 *          (typically a module-scoped static) to maintain double-spawn
 *          protection. Once the task finishes execution or triggers a delete,
 *          the underlying task handle variable must be set back to `NULL` to
 *          allow future respawns.
 *
 * @param[out]    m_ret    Status variable (`esp_err_t`) to store the creation
 *                         result.
 * @param[in,out] m_handle Variable of type `TaskHandle_t` to store the spawned
 *                         task handle.
 * @param[in]     m_alias  The base identifier alias (e.g., `WIFI`, `SENSOR`,
 *                         `MQTT`).
 * @param[in]     m_worker The function pointer to the task entry point (worker
 *                         function).
 *
 * Example of use:
 * @code{c}
 * #include <esp_log.h>
 * #include <freertos/FreeRTOS.h>
 * #include <freertos/task.h>
 * #include <esp_iot_framework_core_ext.h>
 * #include <esp_iot_framework_core_macros.h>
 *
 * // Definitions needed for the 'MQTT' alias
 * #define TASK_MQTT_NAME     "mqtt_service"
 * #define TASK_MQTT_SIZE     4096
 * #define TASK_MQTT_PRIORITY 5
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
 *     // Macro expands to use TASK_MQTT_NAME, TASK_MQTT_SIZE, etc.
 *     EIF_TASK_LAUNCH(ret, mqtt_hdl, MQTT, mqtt_task_worker);
 *    
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
 * Example of use (without double-call protection):
 * @code{c}
 * #include <esp_log.h>
 * #include <freertos/FreeRTOS.h>
 * #include <freertos/task.h>
 * #include <esp_iot_framework_core_ext.h>
 * #include <esp_iot_framework_core_macros.h>
 *
 * // Definitions needed for the 'NAME' alias
 * #define TASK_NAME_NAME     "mqtt_service"
 * #define TASK_NAME_SIZE     4096
 * #define TASK_NAME_PRIORITY 5
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
 *         // Macro expands to use TASK_NAME_NAME, TASK_NAME_SIZE, etc.
 *         EIF_TASK_LAUNCH(ret, hdl, NAME, task_worker);
 *         if (ret != ESP_OK) {
 *             EIF_LOG_E("Failed to launch task iteration %d", i);
 *         }
 *     }
 * }
 * @endcode
 */
#define EIF_TASK_LAUNCH(m_ret, m_handle, m_alias, m_worker) \
do { \
    /* @deviation [Rule 20.10] The '##' token pasting operator is utilized to
     * enforce compile-time synchronization between task aliases and their
     * respective system parameters (NAME, SIZE, PRIORITY). This compile-time
     * binding enforces architecture-level mutual exclusion and prevents runtime
     * mismatch risks. The macro expansion is strictly deterministic and
     * guaranteed to resolve only into valid, well-defined C identifiers.
     */ \
    (m_ret) = eif_task_common_spawn( \
        &(m_handle), \
        (m_worker), \
        TASK_##m_alias##_NAME, \
        TASK_##m_alias##_SIZE, \
        TASK_##m_alias##_PRIORITY \
    ); \
} while (false)
/** @} */

/** @} */

#ifdef __cplusplus
    }
#endif

#endif