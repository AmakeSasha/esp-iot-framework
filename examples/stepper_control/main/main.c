#include <esp_err.h>
#include <esp_http_server.h>
#include <esp_iot_framework_core.h>
#include <esp_iot_framework_device.h>

#include "stepper.h"

esp_err_t motor_dir_handler(httpd_req_t *req) {
    bool real_url = false;

    if (strncmp(req->uri, "/api/motor/up", 13) == 0) {
        g_motor_state.current_dir = UP;
        real_url = true;
    } else if (strncmp(req->uri, "/api/motor/down", 15) == 0) {
        g_motor_state.current_dir = DOWN;
        real_url = true;
    } else if (strncmp(req->uri, "/api/motor/stop", 15) == 0) {
        g_motor_state.current_dir = STOP;
        real_url = true;
    }

    if (real_url) {
        httpd_resp_set_status(req, HTTPD_204);
        httpd_resp_send(req, NULL, 0);
    } else {
        httpd_resp_set_status(req, HTTPD_400);
        httpd_resp_send(req, NULL, 0);
    }

    return ESP_OK;
}

esp_err_t motor_power_handler(httpd_req_t *req) {
    bool real_url = false;

    if (strncmp(req->uri, "/api/motor/on", 13) == 0) {
        g_motor_state.is_powered = true;
        real_url = true;

        // Allow driver capacitors and MOSFETs to fully wake up and stabilize
        // holding current, preventing the motor from skipping the first steps
        // during startup.
        vTaskDelay(pdMS_TO_TICKS(2)); 
    } else if (strncmp(req->uri, "/api/motor/off", 14) == 0) {
        g_motor_state.is_powered = false;
        real_url = true;
    }

    if (real_url) {
        httpd_resp_set_status(req, HTTPD_204);
        httpd_resp_send(req, NULL, 0);
    } else {
        httpd_resp_set_status(req, HTTPD_400);
        httpd_resp_send(req, NULL, 0);
    }

    return ESP_OK;
}

static const httpd_uri_t my_uris[] = {
    /* Dir */
    { .uri = "/api/motor/up",   .method = HTTP_GET, .handler = motor_dir_handler },
    { .uri = "/api/motor/down", .method = HTTP_GET, .handler = motor_dir_handler },
    { .uri = "/api/motor/stop", .method = HTTP_GET, .handler = motor_dir_handler },
    /* Power */
    { .uri = "/api/motor/on",  .method = HTTP_GET, .handler = motor_power_handler },
    { .uri = "/api/motor/off", .method = HTTP_GET, .handler = motor_power_handler }
};

void app_main(void) {
    ESP_ERROR_CHECK(eif_core_initialize());
    ESP_ERROR_CHECK(eif_device_initialize());
    ESP_ERROR_CHECK(eif_nvs_initialize());
    ESP_ERROR_CHECK(eif_set_uri_handlers(my_uris, 5));
    ESP_ERROR_CHECK(eif_wifi_initialize());

    stepper_init();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}