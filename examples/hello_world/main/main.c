/*
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Example: hello_world
 * Folder: ./examples/hello_world/main
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

#include <esp_err.h>
#include <esp_http_server.h>
#include <esp_iot_framework_core.h>
#include <esp_iot_framework_device.h>

esp_err_t hello_world(httpd_req_t *req) {
    const char *resp = "Hello World, from esp_iot_framework!";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static const httpd_uri_t my_uris[] = {
    { .uri = "/hello", .method = HTTP_GET, .handler = hello_world }
};

void app_main(void) {
    ESP_ERROR_CHECK(eif_core_initialize());
    ESP_ERROR_CHECK(eif_device_initialize());
    ESP_ERROR_CHECK(eif_nvs_initialize());
    ESP_ERROR_CHECK(eif_set_uri_handlers(my_uris, sizeof(my_uris) / sizeof(my_uris[0])));
    ESP_ERROR_CHECK(eif_wifi_initialize());

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}