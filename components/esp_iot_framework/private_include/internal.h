/* SPDX-FileCopyrightText: 2026 AmakeSasha
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Path: ./components/esp_iot_framework/private_include/internal.h
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

#ifndef EIF__INTERNAL_H
#define EIF__INTERNAL_H

#include "sdkconfig.h"

#include <mdns.h>
#include <esp_wifi.h>
#ifdef CONFIG_EIF_ENABLE_TLS
    #include <esp_https_server.h>
#else
    #include <esp_http_server.h>
#endif

#include "esp_iot_framework.h"

/* network.h */
#define MDNS_HOSTNAME_MAX_LEN      56U
#define MDNS_INSTANCE_NAME_MAX_LEN 63U
#define MDNS_HOSTNAME_FULL_MAX_LEN 63U
#define MDNS_TXT_RECORDS_MAX_COUNT 32U
/*                                sizeof(".local")*/
#if (MDNS_HOSTNAME_FULL_MAX_LEN - 7U) != MDNS_HOSTNAME_MAX_LEN
    #error "Invalid value `MDNS_HOSTNAME_MAX_LEN`"
#endif

uint8_t wifi_get_current_profile_index(void);
esp_err_t wifi_initialize(void);
bool wifi_is_ssid_available(const char * const ssid);



/* config.c */
/* ESP IoT Framework - eif */
typedef struct {
    /* Wi-Fi */
    wifi_ps_type_t wifi_power_mode;
    /* mDNS */
    char mdns_hostname[MDNS_HOSTNAME_MAX_LEN + 1U];
    char mdns_instance_name[MDNS_INSTANCE_NAME_MAX_LEN + 1U];
    mdns_txt_item_t mdns_txt_records[MDNS_TXT_RECORDS_MAX_COUNT];
    size_t mdns_txt_records_count;
    /* System */
    void (*sys_handler_reboot)(void);
    /* HTTP(S) server */
    httpd_uri_t *server_uri_handlers;
    size_t server_uri_handlers_count;
    #ifdef CONFIG_EIF_ENABLE_TLS
        httpd_ssl_config_t server_config;
    #else
        httpd_config_t     server_config;
    #endif
} eif_config_t;

const eif_config_t* eif_config_get(void);

#ifdef CONFIG_EIF_ENABLE_TLS
    void config_set_tls_cred(
        const char * const cert,     const size_t cert_len,
        const char * const priv_key, const size_t priv_key_len
    );
#endif /* CONFIG_EIF_ENABLE_TLS */



/* http_server.h */
#ifdef CONFIG_EIF_ENABLE_WEB_ADMIN_GUI
    typedef struct {
        const uint8_t * const start;
        const uint8_t * const end;
        const char * const content_type;
        const char * const file_name;
        bool need_cache;
    } eif_web_file_t;
#endif

extern const size_t http_server_default_handlers_count;

esp_err_t server_launch(void);



/* nvs.h */
#define NVS_WIFI_SSID_DEFAULT "ESP32_SETUP"
#define NVS_WIFI_SSID_MIN_LEN 1U
#define NVS_WIFI_SSID_MAX_LEN 31U

#define NVS_WIFI_PASS_DEFAULT "12345678"
#define NVS_WIFI_PASS_MIN_LEN 8U
#define NVS_WIFI_PASS_MAX_LEN 63U

/** Example of the value: 'Basic YWRtaW46' */
#define NVS_BASIC_AUTH_LINE_MIN_LEN 14U
/** Example of the value: 'Basic YWRtaW46MTIzNDU2Nzg5MHF3ZXJ0eTBxd2VydHkwcXdlcnR5MDE=' */
#define NVS_BASIC_AUTH_LINE_MAX_LEN 58U
/** Example of the value: '' */
#define NVS_BASIC_AUTH_PASS_MIN_LEN 0U
/** Example of the value: '1234567890qwerty0qwerty0qwerty01' */
#define NVS_BASIC_AUTH_PASS_MAX_LEN 32U

esp_err_t nvs_initialize(void);

esp_err_t nvs_wifi_profile_save(
    uint8_t index,
    const char (*ssid)[NVS_WIFI_SSID_MAX_LEN + 1U],
    const char (*pass)[NVS_WIFI_PASS_MAX_LEN + 1U]
);
esp_err_t nvs_wifi_profile_load(
    uint8_t index,
    char (*ssid)[NVS_WIFI_SSID_MAX_LEN + 1U],
    char (*pass)[NVS_WIFI_PASS_MAX_LEN + 1U]
);

esp_err_t nvs_basic_auth_line_save(const char (*pass)[NVS_BASIC_AUTH_PASS_MAX_LEN + 1U]);
esp_err_t nvs_basic_auth_line_load(char (*pass)[NVS_BASIC_AUTH_LINE_MAX_LEN + 1U]);

#ifdef CONFIG_EIF_ENABLE_TLS
    esp_err_t nvs_tls_creds_save(const char * const cert, const char * const priv_key);
    esp_err_t nvs_tls_creds_remove(void);
    esp_err_t nvs_tls_creds_load(
        char ** const cert,     size_t * const cert_len,
        char ** const priv_key, size_t * const priv_key_len
    );
#endif /* CONFIG_EIF_ENABLE_TLS */



/* system.c */
#ifdef CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG
    void core_log_init(void);
    size_t core_log_pop_chunk(char *dest_buf, size_t max_size);
#endif /* CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG */

void sys_launch_task_reboot(void);
#ifdef CONFIG_EIF_ENABLE_TLS
    esp_err_t sys_generate_tls_credentials_sync(void);
#endif /* CONFIG_EIF_ENABLE_TLS */
void sys_launch_task_ota_rollback(void);
esp_err_t sys_launch_task_memory_monitor(void);



/* tls_manager.h */
#ifdef CONFIG_EIF_ENABLE_TLS
    esp_err_t tls_create_creds(
        char * * const  cert_pem, size_t * const cert_len,
        char * * const key_pem,   size_t * const key_len
    );
#endif /* CONFIG_EIF_ENABLE_TLS */

#endif /* EIF__INTERNAL_H */