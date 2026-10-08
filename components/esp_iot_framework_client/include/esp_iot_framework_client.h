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

#define EIF_MDNS_MAX_DEVICES 16
#define EIF_MDNS_MAX_HOSTNAME 64
#define EIF_MDNS_TIMEOUT_MS 2000

typedef struct {
    esp_ip_addr_t ip;
    uint16_t port;
    char hostname[EIF_MDNS_MAX_HOSTNAME];
} eif_mdns_device_t;

#ifdef __cplusplus
    extern "C" {
#endif

/**
 * @defgroup client_root Node: CLIENT
 * @copydoc md_docs_html_README_CLIENT
 * @{
 */

/**
 * @brief Scan for mDNS devices
 * @param[in] service Service name (e.g., "_eif_https")
 * @param[out] devices Array to store found devices (caller-allocated)
 * @param[out] out_count Pointer to store the number of found devices
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t eif_mdns_scan(const char *service, eif_mdns_device_t *devices, uint8_t *out_count);

/** @} */

#ifdef __cplusplus
    }
#endif

#endif
