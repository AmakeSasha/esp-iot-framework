/*
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp_iot_framework
 * Folder: include
 * File: esp_iot_framework.h
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

#ifndef ESP_IOT_FRAMEWORK_H
#define ESP_IOT_FRAMEWORK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "esp_err.h"
#include <stdbool.h>
#include "esp_wifi.h"
#include "sdkconfig.h"

/* config.c */
void eif_initialize(void);

typedef void (*eif_pre_reboot_callback_t)(void);
esp_err_t eif_register_pre_reboot_callback(eif_pre_reboot_callback_t cb);

esp_err_t eif_set_wifi_config(
    wifi_init_config_t wifi_driver_config,
    wifi_ps_type_t wifi_power_mode,
    uint32_t wifi_attempt_delay_ms
);
esp_err_t eif_set_wifi_params_count(uint8_t wifi_params_count);

#ifdef CONFIG_EIF_ENABLE_TLS
    #include "esp_https_server.h"
    esp_err_t eif_set_server_config_https(httpd_ssl_config_t *server_config);
#else
    #include "esp_http_server.h"
    esp_err_t eif_set_server_config_http(httpd_config_t *server_config);
#endif

esp_err_t eif_set_uri_handlers(
    httpd_uri_t *uri_handlers, size_t uri_handlers_count
);

#define MDNS_TXT_RECORDS_MAX_COUNT 32
#ifdef CONFIG_EIF_ENABLE_MDNS
    #include "mdns.h"

    esp_err_t eif_set_mdns(
        const char* mdns_hostname, const char* mdns_instance_name
    );
    esp_err_t eif_set_mdns_records(
        const mdns_txt_item_t txt_records[MDNS_TXT_RECORDS_MAX_COUNT], 
        size_t txt_records_count
    );
#else
    struct mdns_txt_item_t;
    typedef struct mdns_txt_item_t mdns_txt_item_t;

    static inline esp_err_t eif_set_mdns(
        const char* mdns_hostname, const char* mdns_instance_name
    ) {
        (void)mdns_hostname;
        (void)mdns_instance_name;
        return ESP_ERR_NOT_SUPPORTED;
    }
    static inline esp_err_t eif_set_mdns_records(
        const mdns_txt_item_t txt_records[MDNS_TXT_RECORDS_MAX_COUNT], 
        size_t txt_records_count
    ) {
        (void)txt_records;
        (void)txt_records_count;
        return ESP_ERR_NOT_SUPPORTED;
    }
#endif

/* nvs.c */
esp_err_t eif_nvs_initialize(void);

/* network.c */
esp_err_t eif_wifi_initialize(void);

#ifdef __cplusplus
}
#endif
#endif