#include <esp_err.h>
#include <esp_http_server.h>
#include <esp_iot_framework_core.h>
#include <esp_iot_framework_device.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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
esp_err_t stepper_dir_handler(httpd_req_t *req) {
    bool real_url = true;

    if (strncmp(req->uri, "/api/stepper/up", 15) == 0) {
        stepper_set_dir(STEPPER_UP);
    } else if (strncmp(req->uri, "/api/stepper/down", 17) == 0) {
        stepper_set_dir(STEPPER_DOWN);
    } else if (strncmp(req->uri, "/api/stepper/stop", 17) == 0) {
        stepper_set_dir(STEPPER_STOP);
    } else {
        real_url = false;
    }

    httpd_resp_set_status(req, real_url ? HTTPD_204 : HTTPD_400);
    httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}

esp_err_t stepper_power_handler(httpd_req_t *req) {
    bool real_url = true;

    if (strncmp(req->uri, "/api/stepper/on", 15) == 0) {
       stepper_set_power(true);

        // Allow driver capacitors and MOSFETs to fully wake up and stabilize
        // holding current, preventing the stepper from skipping the first steps
        // during startup.
        vTaskDelay(pdMS_TO_TICKS(2)); 
    } else if (strncmp(req->uri, "/api/stepper/off", 16) == 0) {
       stepper_set_power(false);
    } else {
        real_url = false;
    }

    httpd_resp_set_status(req, real_url ? HTTPD_204 : HTTPD_400);
    httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}

/*
esp_err_t stepper_json_status_handler(httpd_req_t *req) {
}
*/

/* Main */
static const httpd_uri_t my_uris[] = {
    /* Files */
    { .uri = "/", .method = HTTP_GET, .handler = sendf_index_html },
    /* Dir */
    { .uri = "/api/stepper/up",   .method = HTTP_GET, .handler = stepper_dir_handler },
    { .uri = "/api/stepper/down", .method = HTTP_GET, .handler = stepper_dir_handler },
    { .uri = "/api/stepper/stop", .method = HTTP_GET, .handler = stepper_dir_handler },
    /* Power */
    { .uri = "/api/stepper/on",  .method = HTTP_GET, .handler = stepper_power_handler },
    { .uri = "/api/stepper/off", .method = HTTP_GET, .handler = stepper_power_handler }
};

void app_main(void) {
    ESP_ERROR_CHECK(eif_core_initialize());
    ESP_ERROR_CHECK(eif_device_initialize());
    ESP_ERROR_CHECK(eif_nvs_initialize());
    ESP_ERROR_CHECK(eif_set_uri_handlers(my_uris, 6));
    ESP_ERROR_CHECK(eif_wifi_initialize());

    stepper_init();
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}