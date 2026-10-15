/* SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Path: ./components/esp_iot_framework/private_include/macros.h
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

#ifndef CORE_MACROS_H
#define CORE_MACROS_H

#include "sdkconfig.h"


#if defined(CONFIG_EIF_LOG_SHOW_METADATA) || defined(DOXYGEN)
    #define EIF_PRINT(m_macro, m_format, ...) m_macro(TAG, "%s:%d (%s) " m_format, __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#endif
#if !defined(CONFIG_EIF_LOG_SHOW_METADATA) && !defined(DOXYGEN)
    #define EIF_PRINT(m_macro, m_format, ...) m_macro(TAG, m_format, ##__VA_ARGS__)
#endif

/**
 * @name Logging Severity Levels
 * @{
 */
/** @brief Error log level severity, is level `Error`. */
#define EIF_LOG_LEVEL_E 1
/** @brief Warning log level severity, is level `Warning`. */
#define EIF_LOG_LEVEL_W 2
/** @brief Informational log level severity, is level `Info`. */
#define EIF_LOG_LEVEL_I 3
/** @brief Debug log level severity, is level `Debug`. */
#define EIF_LOG_LEVEL_D 4
/** @} */



/** @brief Logs an error message using the `ESP_LOGE` severity, is level `Error`. */
#if CONFIG_EIF_LOG_LEVEL >= EIF_LOG_LEVEL_E || defined(DOXYGEN)
    #define EIF_LOG_E(...) EIF_PRINT(ESP_LOGE, __VA_ARGS__)
#else
    #define EIF_LOG_E(...) ((void)0U)
#endif
/** @brief Logs a warning message using the `ESP_LOGW` severity, is level `Warning`. */
#if CONFIG_EIF_LOG_LEVEL >= EIF_LOG_LEVEL_W || defined(DOXYGEN)
    #define EIF_LOG_W(...) EIF_PRINT(ESP_LOGW, __VA_ARGS__)
#else
    #define EIF_LOG_W(...) ((void)0U)
#endif
/** @brief Logs an informational message using the `ESP_LOGI` severity, is level `Info`. */
#if CONFIG_EIF_LOG_LEVEL >= EIF_LOG_LEVEL_I || defined(DOXYGEN)
    #define EIF_LOG_I(...) EIF_PRINT(ESP_LOGI, __VA_ARGS__)
#else
    #define EIF_LOG_I(...) ((void)0U)
#endif
/** @brief Logs a debug message using the `ESP_LOGD` severity, is level `Debug`. */
#if CONFIG_EIF_LOG_LEVEL >= EIF_LOG_LEVEL_D || defined(DOXYGEN)
    #define EIF_LOG_D(...) EIF_PRINT(ESP_LOGD, __VA_ARGS__)
#else
    #define EIF_LOG_D(...) ((void)0U)
#endif

#ifdef CONFIG_EIF_ENABLE_WEB_ADMIN_GUI
    #define EIF_DEFINE_HTTP_FILE(m_filename, m_resp_type, m_need_cache) \
        extern const uint8_t m_filename##_start[] asm("_binary_" #m_filename "_start"); \
        extern const uint8_t m_filename##_end[]   asm("_binary_" #m_filename "_end");   \
        static const eif_web_file_t file_##m_filename = { \
            .start        = (m_filename##_start), \
            .end          = (m_filename##_end), \
            .content_type = (m_resp_type), \
            .file_name    = (#m_filename), \
            .need_cache   = (m_need_cache) \
        }; \
        static esp_err_t sendf_##m_filename(httpd_req_t * const req) { \
            return httpd_resp_sendfile(req, &(file_##m_filename)); \
        }
#endif

#if CONFIG_EIF_LOG_LEVEL > EIF_LOG_LEVEL_E || defined(DOXYGEN)
    #define EIF_CHECK_NOT_NULL(m_ptr, m_error) do {       \
        if ((m_ptr) == NULL) {                            \
            EIF_LOG_E("PARAMETER_IS_NULL: '%s'", #m_ptr); \
            ret = (m_error);                              \
            goto cleanup;                                 \
        }                                                 \
    } while(false)
#else
    #define EIF_CHECK_NOT_NULL( m_ptr, m_error) do { \
        if ((m_ptr) == NULL) {                       \
            ret = (m_error);                         \
            goto cleanup;                            \
        }                                            \
    } while(false)
#endif

#define EIF_FAILED(m_expr) (ret = (m_expr) != ESP_OK)

#define EIF_TRY(m_expr) do {          \
    if ((ret = (m_expr)) != ESP_OK) { \
        goto cleanup;                 \
    }                                 \
} while (false)

#define EIF_TRY_INT(m_expr) do { \
    if ((ret = (m_expr)) != 0) { \
        goto cleanup;            \
    }                            \
} while (false)

#define EIF_TRY_CONDITION(m_expr, m_err) do { \
    if ((m_expr)) {                           \
        ret = m_err;                          \
        goto cleanup;                         \
    }                                         \
} while (false)


#endif