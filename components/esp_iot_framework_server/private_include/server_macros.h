/*
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Folder: ./components/esp_iot_framework_server/private_include
 * File: server_macros.h
 * Library: esp_iot_framework_server
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

#ifndef SERVER_MACROS_H
#define SERVER_MACROS_H

#include "sdkconfig.h"
#include <esp_iot_framework_core_macros.h>

/* Other */

/* @deviation [Rule 20.10] The '#' operator is utilized within a dual-stage macro
 * expansion pattern to enforce strict compile-time transformation of macro data
 * into string literals instead of their raw identifier names. Manual replication
 * of these literal values across the subsystem introduces a critical risk of human error
 * (e.g., mismatching configuration constants), leading to out-of-bounds evaluation.
 * The macro expansion is strictly deterministic and guaranteed to produce only valid,
 * well-defined C string literals, eliminating any risk of syntactic corruption or
 * undefined preprocessor evaluation order.
 */
/* cppcheck-suppress misra-c2012-20.10 */
#define EIF_STR_HELPER(x) #x
#define EIF_STR(x) EIF_STR_HELPER(x)

#ifdef CONFIG_EIF_SERVER_ENABLE_WEB_ADMIN_GUI
    /* @deviation [Rule 20.7] Macro parameters inside 'EIF_DEFINE_HTTP_FILE'
     * are intentionally utilized without enclosing parentheses where they are
     * bound to the preprocessor stringification '#' and token-pasting '##'
     * operators. Manual encapsulation of these parameters within parentheses in
     * such contexts violates C standard syntactic rules, resulting in an
     * immediate compilation failure. The macro expansion is safe and strictly
     * deterministic, as it evaluates to valid, well-defined C external symbol
     * declarations, static configurations, and function definitions at
     * compile-time with no risk of operator precedence disruption.
     * @deviation [Rule 20.10] The '#' and '##' operators are utilized to enforce
     * strict compile-time synchronization between external linker-generated
     * binary symbols and their respective C management structures. Manual
     * replication of these symbols introduces a critical risk of human error
     * (e.g., mismatching start/end pointers), leading to undefined memory access.
     * The macro expansion is strictly deterministic and guaranteed to produce
     * only valid, well-defined C identifiers and string literals, eliminating
     * any risk of syntactic corruption or undefined evaluation order.
     */
    /* cppcheck-suppress misra-c2012-20.7 */
    /* cppcheck-suppress misra-c2012-20.10 */
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

#endif