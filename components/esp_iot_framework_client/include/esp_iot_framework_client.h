/*
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Folder: ./components/esp_iot_framework_client/include
 * File: esp_iot_framework_client.h
 * Library: esp_iot_framework_client
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

#ifndef ESP_IOT_FRAMEWORK_CLIENT_H
#define ESP_IOT_FRAMEWORK_CLIENT_H

#include <stdint.h>
#include "lwip/ip_addr.h"

#ifdef __cplusplus
    extern "C" {
#endif

/**
 * @defgroup client_root Node: CLIENT
 * @copydoc md_docs_html_README_CLIENT
 * @{
 */

typedef void (*scan_callback_t)(ip4_addr_t *ip);

void network_scan_ping(uint32_t timeout_ms, scan_callback_t callback);

/** @} */

#ifdef __cplusplus
    }
#endif

#endif