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

#include <string.h>
#include <esp_log.h>
#include <esp_err.h>
#include <mdns.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_idf_version.h"

#include <esp_iot_framework_core.h>
#include <esp_iot_framework_client.h>

void scan_my_mdns_devices(const char * const service) {
    mdns_result_t *results = NULL;
    
    esp_err_t err = mdns_query_ptr(service, "_tcp", 2000, 10, &results);
    
    if (err != ESP_OK || !results) {
        ESP_LOGI("SCAN", "No devices found for service %s", service);
        return;
    }

    mdns_result_t *r = results;
    while (r) {
        if (r->addr) {
            ESP_LOGI("SCAN", "Found OUR device! IP: " IPSTR, IP2STR(&(r->addr->addr.u_addr.ip4)));
        }

        r = r->next;
    }

    mdns_query_results_free(results);
}

void app_main(void) {
    ESP_ERROR_CHECK(eif_core_initialize());
    ESP_ERROR_CHECK(eif_nvs_initialize());
    ESP_ERROR_CHECK(eif_wifi_initialize());

    while (1) {
        scan_my_mdns_devices("_eif_https");
        scan_my_mdns_devices("_eif_http");

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}