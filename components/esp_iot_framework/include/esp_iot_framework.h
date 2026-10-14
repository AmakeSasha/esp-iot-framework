/*
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Folder: ./components/esp_iot_framework_core/include
 * File: esp_iot_framework_core.h
 * Library: esp_iot_framework_core
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

#include <mdns.h>
#include <stdint.h>
#include <stddef.h>
#include <esp_err.h>
#include <stdbool.h>
#include <esp_wifi.h>
#include <esp_http_server.h>
#ifdef CONFIG_EIF_ENABLE_TLS
    #include <esp_https_server.h>
#endif

typedef struct {
    /* Wi-Fi */
    wifi_ps_type_t wifi_power_mode;
    /* mDNS */
    const char *mdns_hostname;
    const char *mdns_instance_name;
    const mdns_txt_item_t *mdns_txt_records;
    size_t mdns_txt_records_count;
    /* System */
    void (*sys_handler_reboot)(void);
    /* HTTP(S) server */
    const httpd_uri_t *server_uri_handlers;
    size_t server_uri_handlers_count;
    #ifdef CONFIG_EIF_ENABLE_TLS
        httpd_ssl_config_t server_cfg;
    #else
        httpd_config_t server_cfg;
    #endif
} eif_init_config_t;

static inline void configure_base_httpd(httpd_config_t *cfg) {
    cfg->max_open_sockets = 4;
    cfg->recv_wait_timeout = 8;
    cfg->send_wait_timeout = 8;
    cfg->lru_purge_enable = true;
    cfg->max_resp_headers = 16;
}

static inline eif_init_config_t eif_init_config_default(void) {
    eif_init_config_t cfg = {
        .wifi_power_mode = WIFI_PS_NONE,
        .mdns_hostname = "eif-device",
        .mdns_instance_name = "Smart Device",
        .mdns_txt_records = NULL,
        .mdns_txt_records_count = 0,
        .sys_handler_reboot = NULL,
        .server_uri_handlers = NULL,
        .server_uri_handlers_count = 0,
    };
    
    #ifdef CONFIG_EIF_ENABLE_TLS
        httpd_ssl_config_t server_cfg_temp = HTTPD_SSL_CONFIG_DEFAULT();
            
        server_cfg_temp.session_tickets = false;
        server_cfg_temp.transport_mode = HTTPD_SSL_TRANSPORT_SECURE;
        server_cfg_temp.port_secure = 443;
        server_cfg_temp.httpd.server_port = 443;
        server_cfg_temp.httpd.stack_size = 10240;
            
        configure_base_httpd(&server_cfg_temp.httpd);
    #else
        httpd_config_t server_cfg_temp = HTTPD_DEFAULT_CONFIG();
            
        server_cfg_temp.server_port = 80;
        server_cfg_temp.stack_size = 4096;
            
        configure_base_httpd(&server_cfg_temp);
    #endif /* CONFIG_EIF_ENABLE_TLS */
    cfg.server_cfg = server_cfg_temp;

    return cfg;
}

#ifdef __cplusplus
    extern "C" {
#endif

esp_err_t eif_initialize(const eif_init_config_t *config);

#ifdef __cplusplus
    }
#endif

#endif /* ESP_IOT_FRAMEWORK_H */