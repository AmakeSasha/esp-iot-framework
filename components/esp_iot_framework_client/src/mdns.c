/*
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Folder: ./components/esp_iot_framework_client/src
 * File: mdns.c
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

#include <mdns.h>
#include <string.h>
#include <esp_log.h>
#include <esp_err.h>
#include "esp_iot_framework_client.h"
#include "esp_iot_framework_core_macros.h"

esp_err_t eif_mdns_scan(const char *service, eif_mdns_device_t *devices, uint8_t *out_count) {
    EIF_TAG_WITH_UNUSED "MDNS_SCAN";
    
    esp_err_t ret = ESP_OK;
    mdns_result_t *results = NULL;
    uint8_t count = 0U;

    EIF_IF_OK_CHECK_NOT_NULL(ret, service, ESP_ERR_INVALID_ARG);
    EIF_IF_OK_CHECK_NOT_NULL(ret, devices, ESP_ERR_INVALID_ARG);
    EIF_IF_OK_CHECK_NOT_NULL(ret, out_count, ESP_ERR_INVALID_ARG);

    if (ret == ESP_OK) {
        *out_count = 0U;

        EIF_IF_OK_CHECK_ESP_ERR_T(ret,
            mdns_query_ptr(service, "_tcp", EIF_MDNS_TIMEOUT_MS, EIF_MDNS_MAX_DEVICES, &results),
            "Failed to discover devices via mDNS for service %s", service);
    }

    if (ret == ESP_OK) {
        if (results == NULL) {
            EIF_LOG_I("No devices found for service %s", service);
        } else {
            for (mdns_result_t *r = results; r && count < EIF_MDNS_MAX_DEVICES; r = r->next) {
                for (mdns_ip_addr_t *addr = r->addr; addr && count < EIF_MDNS_MAX_DEVICES; addr = addr->next) {
                    eif_mdns_device_t *dev = &devices[count];
                    memset(dev, 0, sizeof(*dev));
                    dev->port = (uint16_t)r->port;

                    if (r->hostname) {
                        char *dot = strchr(r->hostname, '.');
                        size_t len = dot ? (size_t)(dot - r->hostname) : strlen(r->hostname);
                        if (len >= sizeof(dev->hostname)) len = sizeof(dev->hostname) - 1U;
                        
                        memcpy(dev->hostname, r->hostname, len);
                        dev->hostname[len] = '\0';
                    }

                    dev->ip = addr->addr; 
                    
                    count++;
                }
            }
        }

        *out_count = count;
    }

    if (results != NULL) {
        mdns_query_results_free(results);
    }

    return ret;
}
