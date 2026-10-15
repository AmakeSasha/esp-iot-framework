/* SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Path: ./components/esp_iot_framework/src/config.c
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

#include <mdns.h>
#include <stdlib.h>
#include <esp_mac.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <esp_ota_ops.h>

#include "private_include/macros.h"
#include "private_include/internal.h"

#define TAG "EIF Config"

/* --- */

static eif_config_t device_cfg = {0};

static esp_err_t check_snprintf_result(int res, size_t limit, const char *field_name) {
    if ((res < 0) || (res >= (int)limit)) {
        EIF_LOG_E("'%s' is too long! Max allowed: %u", field_name, (unsigned int)limit);
        return ESP_ERR_INVALID_SIZE;
    }
    return ESP_OK;
}

/* ===================== */
/*    Framework Entry    */
/* ===================== */
static esp_err_t config_initialize(const eif_init_config_t *config) {
    esp_err_t ret = ESP_OK;

    EIF_CHECK_NOT_NULL(config->server_uri_handlers, ESP_ERR_INVALID_ARG);

    size_t size_array_user_uris = config->server_uri_handlers_count * sizeof(httpd_uri_t);
    size_t handlers_count = http_server_default_handlers_count + config->server_uri_handlers_count;

    device_cfg.server_uri_handlers = pvPortMalloc(size_array_user_uris);
    EIF_CHECK_NOT_NULL(device_cfg.server_uri_handlers, ESP_ERR_NO_MEM);
    (void)memcpy(device_cfg.server_uri_handlers, config->server_uri_handlers, size_array_user_uris);

    device_cfg.sys_handler_reboot = config->sys_handler_reboot;
    device_cfg.wifi_power_mode = config->wifi_power_mode;

    device_cfg.server_uri_handlers_count = config->server_uri_handlers_count;
    device_cfg.server_config = config->server_cfg; 
    #ifdef CONFIG_EIF_ENABLE_TLS
        device_cfg.server_config.httpd.max_uri_handlers = handlers_count;
    #else
        device_cfg.server_config.max_uri_handlers = handlers_count;
    #endif /* CONFIG_EIF_ENABLE_TLS */

    EIF_LOG_I("'Framework configuration' initialized");

cleanup:
    if (ret != ESP_OK) {
        EIF_LOG_E("'Framework configuration' initialization failed, err: %s", esp_err_to_name(ret));
    }
    return ret;
}

static esp_err_t set_mdns_data(const eif_init_config_t *config) {
    uint8_t mac[6];
    esp_err_t ret = ESP_OK;

    EIF_CHECK_NOT_NULL(config->mdns_hostname, ESP_ERR_INVALID_ARG);
    EIF_CHECK_NOT_NULL(config->mdns_instance_name, ESP_ERR_INVALID_ARG);

    EIF_TRY(esp_read_mac(mac, ESP_MAC_WIFI_STA));
    
    if (config->mdns_txt_records_count > 0U) {
        EIF_CHECK_NOT_NULL(config->mdns_txt_records, ESP_ERR_INVALID_ARG);
        /* @note `check_snprintf_result()` performs a check that is also applicable here.
         *       I reused it to keep the code cleaner. */
        EIF_TRY(check_snprintf_result((int)config->mdns_txt_records_count,
                                      MDNS_TXT_RECORDS_MAX_COUNT, "mdns_txt_records"));

        for (size_t i = 0; i < config->mdns_txt_records_count; i++) {
            if ((config->mdns_txt_records[i].key == NULL) ||
                (config->mdns_txt_records[i].value == NULL)
            ) {
                ret = ESP_ERR_INVALID_SIZE;
                EIF_LOG_E("Invalid MDNS record at index %u", (unsigned int)i);
                goto cleanup;
            }
        }

        (void)memcpy((void *)device_cfg.mdns_txt_records, (const void *)config->mdns_txt_records,
                    config->mdns_txt_records_count * sizeof(mdns_txt_item_t));
        device_cfg.mdns_txt_records_count = config->mdns_txt_records_count;
    }

    int len_host = snprintf(device_cfg.mdns_hostname, sizeof(device_cfg.mdns_hostname),
                            "%s-%02x%02x%02x", config->mdns_hostname, mac[3], mac[4], mac[5]);
    /* @note `snprintf` is used here instead of `memcpy` specifically to capture the intended
     *       string length (`len_inst`), allowing us to validate boundaries and detect truncation
     *       downstream using `check_snprintf_result()`. */
    int len_inst = snprintf(device_cfg.mdns_instance_name, sizeof(device_cfg.mdns_instance_name),
                            "%s", config->mdns_instance_name);

    EIF_TRY(check_snprintf_result(len_host,
                                  sizeof(device_cfg.mdns_hostname), "mdns_hostname"));
    EIF_TRY(check_snprintf_result(len_inst,
                                  sizeof(device_cfg.mdns_instance_name), "mdns_instance_name"));

    EIF_LOG_I("'mDNS credentials' initialized, hostname: %s.local", device_cfg.mdns_hostname);

cleanup:
    if (ret != ESP_OK) {
        EIF_LOG_E("'mDNS credentials' initialization failed, err: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t eif_initialize(const eif_init_config_t *config) {
    esp_err_t ret = ESP_OK;

    EIF_CHECK_NOT_NULL(config, ESP_ERR_INVALID_ARG);

    #ifdef CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG
        core_log_init();
    #endif
    
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (running != NULL) {
        esp_ota_img_states_t state = ESP_OTA_IMG_UNDEFINED;
        if (esp_ota_get_state_partition(running, &state) == ESP_OK) {
            EIF_LOG_I("Partition state: %d", (int)state);
        } else {
            EIF_LOG_I("Partition state: unknown");
        }
    }

    EIF_TRY(sys_launch_task_memory_monitor());
    EIF_TRY(config_initialize(config));
    EIF_TRY(set_mdns_data(config));
    EIF_TRY(nvs_initialize());
    EIF_TRY(wifi_initialize());
    
    EIF_LOG_I("EIF initialized successfully! Device is ready.");

cleanup:
    if (ret != ESP_OK) {
        EIF_LOG_E("EIF initialization failed, err: %s", esp_err_to_name(ret));
    }
    return ret;
}
/* end Framework Entry */


/* Private setter */
#ifdef CONFIG_EIF_ENABLE_TLS
    void config_set_tls_cred(
        const char * const cert,     const size_t cert_len,
        const char * const priv_key, const size_t priv_key_len
    ) {
        device_cfg.server_config.cacert_pem  = (const uint8_t *)cert;
        device_cfg.server_config.cacert_len  = cert_len;
        device_cfg.server_config.prvtkey_pem = (const uint8_t *)priv_key;
        device_cfg.server_config.prvtkey_len = priv_key_len;
    }
#endif

/* Private getters */

const eif_config_t* eif_config_get(void) {
    return &device_cfg;
}