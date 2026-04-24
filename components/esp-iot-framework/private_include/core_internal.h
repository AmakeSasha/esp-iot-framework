/*
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp_iot_framework
 * Folder: src
 * File: core_internal.h
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

#ifndef CORE_INTERNAL_H
#define CORE_INTERNAL_H

#include "esp_wifi.h"
#include "sdkconfig.h"
#ifdef CONFIG_EIF_ENABLE_MDNS
    #include "mdns.h"
#endif
#ifdef CONFIG_EIF_ENABLE_TLS
    #include "esp_https_server.h"
#else
    #include "esp_http_server.h"
#endif

#include "esp_iot_framework.h"

/* config.c */

#ifdef CONFIG_EIF_ENABLE_WEB_ADMIN_GUI
    #define NUM_1 7
#else
    #define NUM_1 0
#endif
#ifdef CONFIG_EIF_ENABLE_BASIC_AUTH
    #define NUM_2 1
#else
    #define NUM_2 0
#endif
#ifdef CONFIG_EIF_ENABLE_TLS
    #define NUM_3 1
#else
    #define NUM_3 0
#endif
enum { DEFAULT_HANDLERS_COUNT = (11 + NUM_1 + NUM_2 + NUM_3) };
#undef NUM_1
#undef NUM_2
#undef NUM_3

#define WIFI_PROFILES_MAX_COUNT 7
#if defined(CONFIG_EIF_ENABLE_MDNS) || defined(CONFIG_EIF_ENABLE_TLS)
    #define MDNS_HOSTNAME_FULL_MAX_LEN 64
    #define MDNS_SUFFIX_MAC_LEN 7 /* -aaccbb */
    #define MDNS_HOSTNAME_PREFIX_MAX_LEN MDNS_HOSTNAME_FULL_MAX_LEN - MDNS_SUFFIX_MAC_LEN
#endif
#ifdef CONFIG_EIF_ENABLE_MDNS
    #define MDNS_INSTANCE_NAME_MAX_LEN 64
#endif

typedef struct {
    bool result;
    int8_t rssi;
} wifi_test_result;

/* ESP IoT Framework - eif */
typedef struct {
    /* eif_set_cfg_wifi */
    wifi_init_config_t wifi_driver_config;
    wifi_ps_type_t wifi_power_mode;
    uint32_t wifi_attempt_delay_ms;
    /* eif_set_wifi_profiles_count */
    size_t wifi_profiles_count;

    /* eif_set_uri_handlers */
    httpd_uri_t *uri_handlers;
    size_t uri_handlers_count;

    #ifdef CONFIG_EIF_ENABLE_TLS
        /* eif_set_server_config_https */
        httpd_ssl_config_t server_config;
    #else
        /* eif_set_server_config_http */
        httpd_config_t server_config;
    #endif

    #if defined(CONFIG_EIF_ENABLE_MDNS) || defined(CONFIG_EIF_ENABLE_TLS)
        char mdns_hostname[MDNS_HOSTNAME_FULL_MAX_LEN];
    #endif
    #ifdef CONFIG_EIF_ENABLE_MDNS
        /* eif_set_mdns */
        char mdns_instance_name[MDNS_INSTANCE_NAME_MAX_LEN];
        /* eif_set_mdns_records */
        mdns_txt_item_t mdns_txt_records[MDNS_TXT_RECORDS_MAX_COUNT];
        size_t mdns_txt_records_count;
    #endif

    /* eif_register_pre_reboot_callback */
    eif_pre_reboot_callback_t user_pre_reboot_cb;

    /* Core */
    uint8_t wifi_profile_index;
    bool wifi_handler_stop;
    wifi_test_result wifi_result_tests[WIFI_PROFILES_MAX_COUNT];
} eif_t;

const eif_t* eif_get(void);

esp_err_t eif_wifi_result_test_set(uint8_t index, wifi_test_result result);
esp_err_t eif_wifi_p_index_set(uint8_t index);
void eif_wifi_handler_stop_set(bool stop);
void eif_uri_handlers_count_update(void);

#if defined(CONFIG_EIF_ENABLE_MDNS) || defined(CONFIG_EIF_ENABLE_TLS)
    void eif_format_mdns_hostname();
#endif
#ifdef CONFIG_EIF_ENABLE_TLS
    esp_err_t eif_set_tls_creds_from_nvs();
#endif

/* nvs.c */

#ifdef CONFIG_EIF_ENABLE_TLS
    esp_err_t nvs_tls_creds_load(
        char** cert_out, size_t* cert_out_len, 
        char** key_out, size_t* key_out_len
    );
    esp_err_t nvs_tls_creds_create_and_save(void);
#endif

#define WIFI_DEFAULT_SSID "ESP32_SETUP"
#define WIFI_EMPTY_SSID ""
#define SSID_MAX_LEN 32
#define SSID_MIN_LEN 1

#define WIFI_DEFAULT_PASS "12345678"
#define WIFI_EMPTY_PASS ""
#define PASSWORD_MAX_LEN 64
#define PASSWORD_MIN_LEN 8

esp_err_t nvs_wifi_profile_save(
    uint8_t index, const char* ssid, const char* pass
);
esp_err_t nvs_wifi_profile_load(
    uint8_t index, char* ssid_out, char* pass_out
);

#ifdef CONFIG_EIF_ENABLE_BASIC_AUTH
    /* Base64 full line */
    #define AUTH_LINE_MAX_LEN 52
    #define AUTH_LINE_MIN_LEN 8

    /* String password */
    #define APASS_PASS_MAX_LEN 33
    #define APASS_PASS_MIN_LEN 0

    /* Decryption:
     * - Login:    'admin' 
     * - Password: ''      (yes, without password) */
    #define AUTH_LINE_DEF "Basic YWRtaW46"
    #define _F_AUTH_LINE "web_auth_pass"

    esp_err_t nvs_auth_line_save(const char* pass);
    esp_err_t nvs_auth_line_load(char* out_pass);
#endif

/* web.c */

void eif_server_stop(void);
esp_err_t eif_server_launch(void);

/* cert_maneger.c */

#ifdef CONFIG_EIF_ENABLE_TLS
    void tls_recreate_task(void* arg);
    esp_err_t generate_https_certs(
        char** cert_pem, size_t* cert_len, char** key_pem,  size_t* key_len
    );
#endif

/* network.c */

void wifi_test_task(void *pvParameters);

/* system.c */

void system_reboot_prepare(void);
void reboot_task(void *pvParameters);
void rollback_and_reboot_task(void *pvParameters);
void memory_monitor_task(void *pvParameters);

#endif