/*
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Folder: ./components/esp_iot_framework_device/src
 * File: config.c
 * Library: esp_iot_framework_device
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

#include "sdkconfig.h"

#include <string.h>
#include <stdlib.h>
#include <esp_mac.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_ota_ops.h>
#include <freertos/FreeRTOS.h>
#ifdef CONFIG_EIF_ENABLE_TLS
    #include <esp_https_server.h>
#else
    #include <esp_http_server.h>
#endif

#include "device_internal.h"
#include <esp_iot_framework_core_ext.h>
#include <esp_iot_framework_core_macros.h>

#define TAG "Device config"

/* --- */

#define CFG_ERR_ALLOCATE    "Failed to allocate %d bytes for '%s'"

#if CONFIG_EIF_LOG_LEVEL >= EIF_LOG_LEVEL_D
    #define MSG_CALL_SETTER  "Calling the setter `%s`"
    #define MSG_CALL_UPDATER "Calling the setter `%s`"
#endif

/* --- */

static eif_device_t eif_dev_cfg = {0};

/* ===================== */
/*    Framework Entry    */
/* ===================== */
esp_err_t eif_device_initialize(void) {
    esp_err_t ret = ESP_OK;

    EIF_LOG_D(MSG_CALL_SETTER, __func__);

    EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_register_handler_ip_got(eif_server_launch),
        "Failed to register event handler for `IP_EVENT_STA_GOT_IP`");
    EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_register_handler_ip_lost(eif_server_stop),
        "Failed to register event handler for `IP_EVENT_STA_LOST_IP`");

    #ifdef CONFIG_EIF_ENABLE_TLS
        httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();
        config.cacert_pem = NULL;
        config.cacert_len = 0;
        config.prvtkey_pem = NULL;
        config.prvtkey_len = 0;
        config.session_tickets = false;
        config.transport_mode = HTTPD_SSL_TRANSPORT_SECURE;
        config.httpd.server_port = 443;
        config.port_secure = 443;
        httpd_config_t *cfg_httpd = &config.httpd;
        cfg_httpd->stack_size = 10240;
    #else
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.server_port = 80;
        httpd_config_t *cfg_httpd = &config;
        cfg_httpd->stack_size = 4096;
    #endif

    cfg_httpd->max_open_sockets = 4;
    cfg_httpd->recv_wait_timeout = 8;
    cfg_httpd->send_wait_timeout = 8;
    cfg_httpd->lru_purge_enable = true;
    cfg_httpd->max_resp_headers = 16;
    cfg_httpd->max_uri_handlers = 32;

    eif_device_t temp_config = {
        .uri_handlers = NULL,
        .uri_handlers_count = 0,
        .server_config = config
    };

    eif_dev_cfg = temp_config;
    EIF_LOG_I("Configuration initialized successfully");

    /* Cleanup */
    return ret;
}

/* Public setters */

/* ================================== */
/*    HTTP(S) Server Configuration    */
/* ================================== */
#ifdef CONFIG_EIF_ENABLE_TLS
    esp_err_t eif_set_server_config_https(
        const httpd_ssl_config_t * const server_config
    ) {
        EIF_LOG_D(MSG_CALL_SETTER, __func__);

        esp_err_t ret = ESP_OK;

        EIF_IF_OK_CHECK_NOT_NULL(ret, server_config, ESP_ERR_INVALID_ARG);
       
        if (ret == ESP_OK) {
            (void)memcpy(&eif_dev_cfg.server_config, server_config, sizeof(httpd_ssl_config_t));

            if (eif_dev_cfg.server_config.cacert_pem) {
                vPortFree((void *)eif_dev_cfg.server_config.cacert_pem);
            }
            eif_dev_cfg.server_config.cacert_pem = NULL;
            eif_dev_cfg.server_config.cacert_len = 0;

            if (eif_dev_cfg.server_config.prvtkey_pem) {
                vPortFree((void *)eif_dev_cfg.server_config.prvtkey_pem);
            }
            eif_dev_cfg.server_config.prvtkey_pem = NULL;
            eif_dev_cfg.server_config.prvtkey_len = 0;

            #ifdef CONFIG_EIF_ENABLE_TLS
                eif_dev_cfg.server_config.transport_mode = HTTPD_SSL_TRANSPORT_SECURE;
                eif_dev_cfg.server_config.session_tickets = false;
            #endif
        }

        /* Cleanup */
        return ret;
    }
#else
    esp_err_t eif_set_server_config_http(
        const httpd_config_t * const server_config
    ) {
        esp_err_t ret = ESP_OK;

        EIF_LOG_D(MSG_CALL_SETTER, __func__);

        EIF_IF_OK_CHECK_NOT_NULL(ret, server_config, ESP_ERR_INVALID_ARG);
        if (ret == ESP_OK) {
            memcpy(&eif_dev_cfg.server_config, server_config, sizeof(httpd_config_t));
        }

        /* Cleanup */
        return ret;
    }
#endif

esp_err_t eif_set_uri_handlers(
    const httpd_uri_t * const uri_handlers, size_t uri_handlers_count
) {
    esp_err_t ret = ESP_OK;

    EIF_LOG_D(MSG_CALL_SETTER, __func__);

    EIF_IF_OK_CHECK_NOT_NULL(ret, uri_handlers, ESP_ERR_INVALID_ARG);

    if (ret == ESP_OK) {
        if (eif_dev_cfg.uri_handlers) {
            vPortFree(eif_dev_cfg.uri_handlers);
        }
        if (uri_handlers_count == 0U) {
            eif_dev_cfg.uri_handlers = NULL;
            eif_dev_cfg.uri_handlers_count = 0U;
            EIF_LOG_W("No handlers to copy (count = 0)");
        }
    }

    if ((ret == ESP_OK) && (uri_handlers_count > 0U)) {
        size_t size_array_uris = uri_handlers_count * sizeof(httpd_uri_t);

        eif_dev_cfg.uri_handlers = pvPortMalloc(size_array_uris);
        EIF_IF_OK_CHECK_NOT_NULL(ret, eif_dev_cfg.uri_handlers, ESP_ERR_NO_MEM);

        if (ret == ESP_OK) {
            (void)memcpy(eif_dev_cfg.uri_handlers, uri_handlers, size_array_uris);
            eif_dev_cfg.uri_handlers_count = uri_handlers_count;
        }
    }

    /* Cleanup */
    if ((ret != ESP_OK) && (eif_dev_cfg.uri_handlers)) {
        vPortFree(eif_dev_cfg.uri_handlers);
        eif_dev_cfg.uri_handlers = NULL;
        eif_dev_cfg.uri_handlers_count = 0U;
    }
    return ret;
}
/* end HTTP(S) Server Configuration */



/* Private setters */
void eif_uri_handlers_count_update(void) {
    EIF_LOG_D(MSG_CALL_UPDATER, __func__);

    size_t count = eif_dev_cfg.uri_handlers_count + DEFAULT_HANDLERS_COUNT;
    #ifdef CONFIG_EIF_ENABLE_TLS
        eif_dev_cfg.server_config.httpd.max_uri_handlers = count;
    #else
        eif_dev_cfg.server_config.max_uri_handlers = count;
    #endif
}

#ifdef CONFIG_EIF_ENABLE_TLS
    esp_err_t eif_set_tls_creds_from_nvs(void) {
        esp_err_t ret = ESP_OK;
        char *c_buf = NULL;
        char *k_buf = NULL;
        size_t c_len = 0;
        size_t k_len = 0;
        uint8_t *c_copy = NULL;
        uint8_t *k_copy = NULL;

        EIF_LOG_D(MSG_CALL_SETTER, __func__);

        EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_nvs_value_load_malloc(
            EIF_NVS_KEY_TLS_CERT, &c_buf, &c_len), "Failed loaded 'TLS cert'");
        EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_nvs_value_load_malloc(
            EIF_NVS_KEY_TLS_PRIV_KEY, &k_buf, &k_len), "Failed loaded 'TLS key'");
        
        if (ret == ESP_OK) {
            EIF_LOG_I("'TLS credentials' loaded successfully");
            EIF_LOG_I("Cert: %u, Key: %u", (unsigned int)c_len, (unsigned int)k_len);
        }

        c_copy = (uint8_t *)pvPortMalloc(c_len);
        if (c_copy == NULL) {
            EIF_LOG_E(CFG_ERR_ALLOCATE, c_len, "c_copy");
            ret = ESP_ERR_NO_MEM;
        }
        if (ret == ESP_OK) {
            k_copy = (uint8_t *)pvPortMalloc(k_len);
            if (k_copy == NULL) {
                EIF_LOG_E(CFG_ERR_ALLOCATE, k_len, "k_copy");
                ret = ESP_ERR_NO_MEM;
            }
        }

        if (ret == ESP_OK) {
            (void)memcpy(c_copy, c_buf, c_len);
            (void)memcpy(k_copy, k_buf, k_len);

            if (eif_dev_cfg.server_config.cacert_pem != NULL) {
                vPortFree((void *)eif_dev_cfg.server_config.cacert_pem);
            }
            if (eif_dev_cfg.server_config.prvtkey_pem != NULL) {
                vPortFree((void *)eif_dev_cfg.server_config.prvtkey_pem);
            }
            
            eif_dev_cfg.server_config.cacert_pem = c_copy;
            eif_dev_cfg.server_config.cacert_len = c_len;
            eif_dev_cfg.server_config.prvtkey_pem = k_copy;
            eif_dev_cfg.server_config.prvtkey_len = k_len;

            EIF_LOG_I("TLS credentials successfully applied to config");
        }


        /* Cleanup */
        if (ret != ESP_OK) {
            if (c_copy != NULL) {
                vPortFree(c_copy);
            }
            if (k_copy != NULL) {
                vPortFree(k_copy);
            }
        }

        if (c_buf != NULL) {
            vPortFree(c_buf);
        }
        if (k_buf != NULL) {
            vPortFree(k_buf);
        }

        return ret;
    }
#endif

/* Private getters */

const eif_device_t* eif_device_get(void) {
    EIF_LOG_D(MSG_CALL_GETTER, __func__);

    return &eif_dev_cfg;
}