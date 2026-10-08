/*
 * SPDX-License-Identifier: Apache-2.0
 * Example: hello_world_client
 * Folder: ./examples/hello_world_client/main
 * File: main.c
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

#include <esp_log.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_iot_framework_core.h>
#include <esp_iot_framework_client.h>
#include <esp_iot_framework_core_macros.h>

static void scan_devices(const char *service) {
    EIF_TAG_WITH_UNUSED "SCAN";

    esp_err_t ret = ESP_OK;
    eif_mdns_device_t devices[EIF_MDNS_MAX_DEVICES];
    uint8_t count = 0U;

    ret = eif_mdns_scan(service, devices, &count);

    if (ret == ESP_OK) {
        for (uint8_t i = 0U; i < count; i++) {
            EIF_LOG_I("Found device: %s", devices[i].hostname);
            if (devices[i].ip.type == ESP_IPADDR_TYPE_V6) {
                EIF_LOG_I("          IP: " IPV6STR, IPV62STR(devices[i].ip.u_addr.ip6));
            } else {
                EIF_LOG_I("          IP: " IPSTR, IP2STR(&(devices[i].ip.u_addr.ip4)));
            }
        }
    } else {
        EIF_LOG_I("No devices found for service %s", service);
    }
}

void app_main(void) {
    ESP_ERROR_CHECK(eif_core_initialize());
    ESP_ERROR_CHECK(eif_nvs_initialize());
    ESP_ERROR_CHECK(eif_wifi_initialize());

    while (1) {
        scan_devices("_eif_https");
        scan_devices("_eif_http");
        vTaskDelay(pdMS_TO_TICKS((uint32_t)5000));
    }
}
