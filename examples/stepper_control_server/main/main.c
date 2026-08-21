/*
 * SPDX-License-Identifier: Apache-2.0
 * Example: stepper_control_server
 * Folder: ./examples/stepper_control_server/main
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
#include <esp_iot_framework_server.h>

#include "stepper.h"

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

esp_err_t get_count_steps_from_req(httpd_req_t *req, uint32_t *steps_out) {
    esp_err_t ret = ESP_OK;

    char *endptr = NULL;
    char steps_param[16] = {0};

    ret = httpd_req_get_hdr_value_str(req, "X-Steps", steps_param, sizeof(steps_param));
    if (ret == ESP_OK) {
        uint32_t steps = (uint32_t)strtoul(steps_param, &endptr, 10);

        if (endptr == steps_param || *endptr != '\0') {
            ret = ESP_ERR_INVALID_ARG;
        } else {
            *steps_out = steps;
        }
    } else if (ret == ESP_ERR_NOT_FOUND) {
        ret = ESP_OK;
    } else { ; }

    return ret;
}

esp_err_t stepper_dir_handler(httpd_req_t *req) {
    esp_err_t ret = ESP_OK;

    uint32_t steps_out = 0;

    if (strncmp(req->uri, "/api/stepper/dir/up.do", 22) == 0) {
        ret = get_count_steps_from_req(req, &steps_out);
        if (ret == ESP_OK) {
            stepper_set_steps_to_move(steps_out);
            stepper_set_dir(STEPPER_UP);
        }
    } else if (strncmp(req->uri, "/api/stepper/dir/down.do", 24) == 0) {
        ret = get_count_steps_from_req(req, &steps_out);
        if (ret == ESP_OK) {
            stepper_set_steps_to_move(steps_out);
            stepper_set_dir(STEPPER_DOWN);
        }
    } else if (strncmp(req->uri, "/api/stepper/dir/stop.do", 24) == 0) {
        stepper_set_dir(STEPPER_STOP);
    } else {
        ret = ESP_ERR_INVALID_ARG;
    }

    httpd_resp_set_status(req, (ret == ESP_OK) ? HTTPD_204 : HTTPD_400);
    httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}

esp_err_t stepper_power_handler(httpd_req_t *req) {
    bool real_url = true;

    if (strncmp(req->uri, "/api/stepper/power/on.do", 24) == 0) {
       stepper_set_power(true);

        // Allow driver capacitors and MOSFETs to fully wake up and stabilize
        // holding current, preventing the stepper from skipping the first steps
        // during startup.
        vTaskDelay(pdMS_TO_TICKS(2)); 
    } else if (strncmp(req->uri, "/api/stepper/power/off.do", 25) == 0) {
       stepper_set_power(false);
    } else {
        real_url = false;
    }

    httpd_resp_set_status(req, real_url ? HTTPD_204 : HTTPD_400);
    httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}

#define JSON_BUF_LEN 128

esp_err_t stepper_status_handler(httpd_req_t *req) {
    json_gen_str_t jgen = {0};
    char json_buffer[JSON_BUF_LEN] = {0};

    const volatile stepper_state_t * const cfg = stepper_get_cfg();
    char* current_dir = stepper_dir_to_str(cfg->current_dir);

    json_gen_str_start(&jgen, json_buffer, JSON_BUF_LEN, NULL, NULL);
    /* { */
    json_gen_start_object(&jgen);
    /*   "current_dir": "UP", */
    json_gen_obj_set_string(&jgen, "current_dir", current_dir);
    /*   "is_powered": true, */
    json_gen_obj_set_bool(&jgen, "is_powered", cfg->is_powered);
    /*   "step_counter": 12345, */
    json_gen_obj_set_int(&jgen, "step_counter", cfg->step_counter);
    /*   "steps_to_move": 995, */
    json_gen_obj_set_int(&jgen, "steps_to_move", cfg->steps_to_move);
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
    { .uri = "/api/stepper/dir/up.do",   .method = HTTP_POST, .handler = stepper_dir_handler },
    { .uri = "/api/stepper/dir/down.do", .method = HTTP_POST, .handler = stepper_dir_handler },
    { .uri = "/api/stepper/dir/stop.do", .method = HTTP_POST, .handler = stepper_dir_handler },
    /* Power */
    { .uri = "/api/stepper/power/on.do",  .method = HTTP_POST, .handler = stepper_power_handler },
    { .uri = "/api/stepper/power/off.do", .method = HTTP_POST, .handler = stepper_power_handler },
    /* JSON */
    { .uri = "/api/stepper/status.json", .method = HTTP_GET, .handler = stepper_status_handler }
};

esp_err_t reboot_logic(void) {
    stepper_set_dir(STEPPER_STOP);
    stepper_set_power(false);

    return ESP_OK;
}

void app_main(void) {
    ESP_ERROR_CHECK(eif_core_initialize());
    ESP_ERROR_CHECK(eif_register_handler_system_reboot(reboot_logic));
    ESP_ERROR_CHECK(eif_server_initialize());
    ESP_ERROR_CHECK(eif_nvs_initialize());
    ESP_ERROR_CHECK(eif_server_set_uri_handlers(my_uris, sizeof(my_uris) / sizeof(my_uris[0])));
    ESP_ERROR_CHECK(eif_wifi_initialize());

    stepper_init();
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}