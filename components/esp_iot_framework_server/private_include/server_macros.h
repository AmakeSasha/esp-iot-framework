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

#define EIF_STR_HELPER(x) #x
#define EIF_STR(x) EIF_STR_HELPER(x)

#ifdef CONFIG_EIF_SERVER_ENABLE_WEB_ADMIN_GUI
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