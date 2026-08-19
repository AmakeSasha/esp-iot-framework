/*
 * SPDX-License-Identifier: Apache-2.0
 * Example: relay_control
 * Folder: ./examples/relay_control/main
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
#include <json_generator.h>
#include <esp_http_server.h>
#include <esp_iot_framework_core.h>
#include <esp_iot_framework_device.h>

#include "relay.h"

/* Files */
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");

esp_err_t sendf_index_html(httpd_req_t *req) {
    size_t file_size = index_html_end - index_html_start;

    httpd_resp_set_type(req, "text/html; charset=utf-8");

    (void)httpd_resp_send(req, (const char *)index_html_start, file_size);

    return ESP_OK;
}

/* API */

esp_err_t relay_state_handler(httpd_req_t *req) {
    esp_err_t ret = ESP_OK;

    if (strncmp(req->uri, "/api/relay/on.do", 16) == 0) {
        relay_set_state(PIN_ON);
    } else if (strncmp(req->uri, "/api/relay/off.do", 17) == 0) {
        relay_set_state(PIN_OFF);
    } else if (strncmp(req->uri, "/api/relay/toggle.do", 20) == 0) {
        relay_toggle();
    } else if (strncmp(req->uri, "/api/relay/toggle_logic.do", 26) == 0) {
        relay_set_inversed_logic();
    } else {
        ret = ESP_ERR_INVALID_ARG;
    }

    httpd_resp_set_status(req, (ret == ESP_OK) ? HTTPD_204 : HTTPD_400);
    httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}

#define JSON_BUF_LEN 128

esp_err_t relay_status_handler(httpd_req_t *req) {
    json_gen_str_t jgen = {0};
    char json_buffer[JSON_BUF_LEN] = {0};

    const volatile relay_state_t * const cfg = relay_get_cfg();
    const char *state = "";

    if (cfg->is_inversed) {
        state = cfg->is_closed ? "PIN_OFF" : "PIN_ON";
    } else {
        state = cfg->is_closed ? "PIN_ON" : "PIN_OFF";
    }

    json_gen_str_start(&jgen, json_buffer, JSON_BUF_LEN, NULL, NULL);
    /* { */
    json_gen_start_object(&jgen);
    /*   "ligoc_str": "INVERTED", */
    json_gen_obj_set_string(&jgen, "ligoc_str", cfg->is_inversed ? "INVERTED" : "DIRECT");
    /*   "state_str": "PIN_ON", */
    json_gen_obj_set_string(&jgen, "state_str", state);
    /*   "number_of_changes": 24 */
    json_gen_obj_set_int(&jgen, "number_of_changes", cfg->number_of_changes);
    /* } */
    json_gen_end_object(&jgen);

    int json_len = json_gen_str_end(&jgen);
    bool is_err = ((json_len <= 0) || (json_len > JSON_BUF_LEN));

    if (is_err) {
        httpd_resp_set_status(req, HTTPD_500);
        httpd_resp_send(req, NULL, 0);
    } else {
        httpd_resp_set_type(req, "application/json; charset=UTF-8");
        httpd_resp_sendstr(req, json_buffer);
    }

    return is_err ? ESP_FAIL : ESP_OK;
}

/* Main */
static const httpd_uri_t my_uris[] = {
    /* Files */
    { .uri = "/", .method = HTTP_GET, .handler = sendf_index_html },
    /* Dir */
    { .uri = "/api/relay/on.do",           .method = HTTP_POST, .handler = relay_state_handler },
    { .uri = "/api/relay/off.do",          .method = HTTP_POST, .handler = relay_state_handler },
    { .uri = "/api/relay/toggle.do",       .method = HTTP_POST, .handler = relay_state_handler },
    { .uri = "/api/relay/toggle_logic.do", .method = HTTP_POST, .handler = relay_state_handler },
    /* JSON */
    { .uri = "/api/relay/status.json", .method = HTTP_GET, .handler = relay_status_handler }
};

esp_err_t reboot_logic(void) {
    relay_set_state(PIN_OFF);

    return ESP_OK;
}

void app_main(void) {
    ESP_ERROR_CHECK(eif_core_initialize());
    ESP_ERROR_CHECK(eif_register_handler_system_reboot(reboot_logic));
    ESP_ERROR_CHECK(eif_device_initialize());
    ESP_ERROR_CHECK(eif_nvs_initialize());
    ESP_ERROR_CHECK(eif_set_uri_handlers(my_uris, sizeof(my_uris) / sizeof(my_uris[0])));
    ESP_ERROR_CHECK(eif_wifi_initialize());

    relay_init(true);
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}