/*
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Folder: ./components/esp_iot_framework_device/private_include
 * File: device_internal.h
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

#ifndef DEVICE_INTERNAL_H
#define DEVICE_INTERNAL_H

#include <sdkconfig.h>

#ifdef CONFIG_EIF_ENABLE_TLS
    #include <esp_https_server.h>
#else
    #include <esp_http_server.h>
#endif

#include "esp_iot_framework_device.h"

/* config.c */

enum {
    DEFAULT_HANDLERS_COUNT = 11
    #ifdef CONFIG_EIF_ENABLE_WEB_ADMIN_GUI
        + 7
    #endif
    #ifdef CONFIG_EIF_ENABLE_BASIC_AUTH
        + 1
    #endif
    #ifdef CONFIG_EIF_ENABLE_TLS
        + 1
    #endif
    #ifdef CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG
        + 1
    #endif
};

/* ESP IoT Framework - eif */
typedef struct {
    /* eif_set_uri_handlers */
    httpd_uri_t *uri_handlers;
    size_t uri_handlers_count;

    #ifdef CONFIG_EIF_ENABLE_TLS
        /* eif_set_server_config_https */
        httpd_ssl_config_t server_config;
    #else
        /* eif_set_server_config_http */
        httpd_config_t server_config;
    #endif
} eif_device_t;

#ifdef CONFIG_EIF_ENABLE_WEB_ADMIN_GUI
    typedef struct {
        const uint8_t * const start;
        const uint8_t * const end;
        const char * const content_type;
        const char * const file_name;
        bool need_cache;
    } eif_web_file_t;
#endif

const eif_device_t* eif_device_get(void);

void eif_uri_handlers_count_update(void);

#ifdef CONFIG_EIF_ENABLE_TLS
    esp_err_t eif_set_tls_creds_from_nvs(void);
#endif

/* web.c */

esp_err_t eif_server_stop(void);
esp_err_t eif_server_launch(void);

#endif