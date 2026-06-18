/* SPDX-License-Identifier: CC0-1.0
 *
 * This example code is in the `Public Domain` (or `CC0 1.0` licensed, at your option).
 */

#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_iot_framework_core.h"
#include "esp_iot_framework_device.h"

esp_err_t hello_world(httpd_req_t *req) {
    const char *resp = "Hello World, from esp_iot_framework!";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static const httpd_uri_t my_uris[] = {
    { .uri = "/hello", .method = HTTP_GET,  .handler = hello_world }
};

void app_main(void) {
    ESP_ERROR_CHECK(eif_core_initialize());
    ESP_ERROR_CHECK(eif_device_initialize());
    ESP_ERROR_CHECK(eif_nvs_initialize());
    ESP_ERROR_CHECK(eif_set_uri_handlers(my_uris, 1));
    ESP_ERROR_CHECK(eif_wifi_initialize());


    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}