/*
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Folder: ./components/esp_iot_framework_core/private_include
 * File: core_macros.h
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

#ifndef CORE_MACROS_H
#define CORE_MACROS_H

#include "sdkconfig.h"

#if defined(CONFIG_EIF_CORE_ENABLE_TLS) || defined(DOXYGEN)
    /**
     * @brief Executes and validates an MbedTLS expression sequentially.
     *
     * @note Only available if the `Kconfig` option <code>
     *         <a href="group__core__kconfig.html#CONFIG_EIF_CORE_ENABLE_TLS">
     *           CONFIG_EIF_CORE_ENABLE_TLS
     *         </a>
     *       </code> is enabled.
     *
     * This macro operates similarly to `EIF_IF_OK_CHECK_ESP_ERR_T()`, but is
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

#endif