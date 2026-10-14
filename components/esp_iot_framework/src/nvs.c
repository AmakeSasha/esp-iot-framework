/*
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Folder: ./components/esp_iot_framework_core/src
 * File: nvs.c
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

#include "sdkconfig.h"

#include <nvs.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_err.h>
#include <nvs_flash.h>
#include <mbedtls/base64.h>
#ifdef CONFIG_EIF_ENABLE_TLS
    #include <bootloader_random.h>
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
#endif

#include "private_include/macros.h"
#include "private_include/internal.h"

#define NVS_STORAGE_NAME "__nvs_eif_"
#define NVS_CLEAN_HANDLE ((nvs_handle_t)0U)

/* --- */

#define TAG "EIF NVS"

static inline esp_err_t nvs_check_range(
    size_t size, size_t min_len, size_t max_len, bool it_can_be_empty
) {
    esp_err_t ret = ESP_OK;

    if (!(it_can_be_empty && (size == 0U))) {
        if ((size < min_len) || (size > max_len)) {
            ret = ESP_ERR_INVALID_SIZE;
        }
    }

    /* Cleanup */
    return ret;
}

/* --- */

static esp_err_t nvs_value_save(const char * const key, const char * const value) {
    esp_err_t ret = ESP_OK;
    nvs_handle_t handle = NVS_CLEAN_HANDLE;

    EIF_CHECK_NOT_NULL(key, ESP_ERR_INVALID_ARG);
    EIF_CHECK_NOT_NULL(value, ESP_ERR_INVALID_ARG);

    EIF_TRY(nvs_open(NVS_STORAGE_NAME, NVS_READWRITE, &handle));
    EIF_TRY(nvs_set_str(handle, key, value));
    EIF_TRY(nvs_commit(handle));

    EIF_LOG_I("NVS record for key '%s' saved", key);

cleanup:
    if (handle != NVS_CLEAN_HANDLE) {
        (void)nvs_close(handle);
    }
    if (ret != ESP_OK) {
        EIF_LOG_E("NVS [write] operation failed for key '%s', err: %s", 
            key, esp_err_to_name(ret));
    }
    return ret;
}

static esp_err_t nvs_value_save2(
    const char * const key1, const char * const value1,
    const char * const key2, const char * const value2
) {
    esp_err_t ret = ESP_OK;
    nvs_handle_t handle = NVS_CLEAN_HANDLE;

    EIF_CHECK_NOT_NULL(key1, ESP_ERR_INVALID_ARG);
    EIF_CHECK_NOT_NULL(value1, ESP_ERR_INVALID_ARG);
    EIF_CHECK_NOT_NULL(key2, ESP_ERR_INVALID_ARG);
    EIF_CHECK_NOT_NULL(value2, ESP_ERR_INVALID_ARG);

    EIF_TRY(nvs_open(NVS_STORAGE_NAME, NVS_READWRITE, &handle));
    EIF_TRY(nvs_set_str(handle, key1, value1));
    EIF_TRY(nvs_set_str(handle, key2, value2));
    EIF_TRY(nvs_commit(handle));

    EIF_LOG_I("NVS record for keys '%s', '%s' saved", key1, key2);

cleanup:
    if (handle != NVS_CLEAN_HANDLE) {
        (void)nvs_close(handle);
    }
    if (ret != ESP_OK) {
        EIF_LOG_E("NVS [write] operation failed for keys '%s', '%s', err: %s", 
                  key1, key2, esp_err_to_name(ret));
    }
    return ret;
}

static esp_err_t nvs_value_load(
    const char * const key, char * const value_out, size_t max_len
) {
    esp_err_t ret = ESP_OK;
    size_t length = max_len;
    nvs_handle_t handle = NVS_CLEAN_HANDLE;

    EIF_CHECK_NOT_NULL(key, ESP_ERR_INVALID_ARG);
    EIF_CHECK_NOT_NULL(value_out, ESP_ERR_INVALID_ARG);

    EIF_TRY(nvs_open(NVS_STORAGE_NAME, NVS_READONLY, &handle));
    EIF_TRY(nvs_get_str(handle, key, value_out, &length));

    EIF_LOG_I("NVS record for key '%s' loaded", key);

cleanup:
    if (handle != NVS_CLEAN_HANDLE) {
        (void)nvs_close(handle);
    }
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        EIF_LOG_W("NVS record for key '%s' does not exist", key); 
    } else if (ret != ESP_OK) {
        EIF_LOG_E("NVS [read] operation failed for key '%s', err: %s", 
                  key, esp_err_to_name(ret));
    }
    return ret;
}

#ifdef CONFIG_EIF_ENABLE_TLS
    static esp_err_t nvs_value_load_malloc(
        const char * key, char ** const value_out, size_t * const value_out_len
    ) {
        size_t length = 0U;
        esp_err_t ret = ESP_OK;
        nvs_handle_t handle = NVS_CLEAN_HANDLE;

        EIF_CHECK_NOT_NULL(key, ESP_ERR_INVALID_ARG);
        EIF_CHECK_NOT_NULL(value_out, ESP_ERR_INVALID_ARG);
        EIF_CHECK_NOT_NULL(value_out_len, ESP_ERR_INVALID_ARG);

        if (*value_out != NULL) {
            vPortFree(*value_out);
        }
        *value_out = NULL;
        *value_out_len = 0U;

        EIF_TRY(nvs_open(NVS_STORAGE_NAME, NVS_READONLY, &handle));
        EIF_TRY(nvs_get_str(handle, key, NULL, &length));
        
        *value_out = (char *)pvPortMalloc(length);
        if (*value_out == NULL) {
            ret = ESP_ERR_NO_MEM;
            goto cleanup;
        }
        
        EIF_TRY(nvs_get_str(handle, key, *value_out, &length));

        *value_out_len = length;
        EIF_LOG_I("NVS record for key '%s' loaded", key);

    cleanup:
        if (handle != NVS_CLEAN_HANDLE) {
            (void)nvs_close(handle);
        }
        if (ret != ESP_OK) {
            if (*value_out != NULL) {
                vPortFree(*value_out);
                *value_out = NULL;
            }
            *value_out_len = 0U;

            if (ret == ESP_ERR_NVS_NOT_FOUND) {
                EIF_LOG_W("NVS record for key '%s' does not exist", key);
            } else if (ret == ESP_ERR_NO_MEM) {
                EIF_LOG_E("Failed to allocate %u bytes for NVS key '%s'", (unsigned int)length, key);
            } else {
                EIF_LOG_E("NVS [read malloc] operation failed for key '%s', err: %s", 
                        key, esp_err_to_name(ret));
            }
        }
        return ret;
    }
#endif



/* --- */
/* "wifi_ssid" (9) + max index "FF" (2) = 11 */
#define WIFI_KEY_LEN (11U)

static inline void nvs_wifi_gen_keys(
    uint8_t index,
    char (*ssid_buf)[WIFI_KEY_LEN + 1U],
    char (*pass_buf)[WIFI_KEY_LEN + 1U]
) {
    (void)snprintf(*ssid_buf, WIFI_KEY_LEN + 1U, "wifi_ssid%02X", index);
    (void)snprintf(*pass_buf, WIFI_KEY_LEN + 1U, "wifi_pass%02X", index);
}

esp_err_t nvs_wifi_profile_save(
    uint8_t index,
    const char (*ssid)[NVS_WIFI_SSID_MAX_LEN + 1U],
    const char (*pass)[NVS_WIFI_PASS_MAX_LEN + 1U]
) {
    esp_err_t ret = ESP_OK;
    char ssid_key[WIFI_KEY_LEN + 1U] = {0};
    char pass_key[WIFI_KEY_LEN + 1U] = {0};

    EIF_CHECK_NOT_NULL(ssid, ESP_ERR_INVALID_ARG);
    EIF_CHECK_NOT_NULL(pass, ESP_ERR_INVALID_ARG);

    /* @note The Wi-Fi profile with index 0 is hard-coded in the code (read-only) and should not
     *       be overwritten or deleted.Thus, the valid user-accessible range starts from 1. */
    EIF_TRY(nvs_check_range(index, 1U, CONFIG_EIF_CUSTOM_WIFI_PROFILES_COUNT, false));

    size_t ssid_len = strnlen(*ssid, NVS_WIFI_SSID_MAX_LEN);
    size_t pass_len = strnlen(*pass, NVS_WIFI_PASS_MAX_LEN);
    bool it_can_be_empty = (ssid_len == 0U) && (pass_len == 0U);

    EIF_TRY(nvs_check_range(ssid_len, NVS_WIFI_SSID_MIN_LEN,
                            NVS_WIFI_SSID_MAX_LEN, it_can_be_empty));
    EIF_TRY(nvs_check_range(pass_len, NVS_WIFI_PASS_MIN_LEN,
                            NVS_WIFI_PASS_MAX_LEN, it_can_be_empty));
    
    nvs_wifi_gen_keys(index, &ssid_key, &pass_key);
    EIF_TRY(nvs_value_save2(ssid_key, *ssid, pass_key, *pass));

    EIF_LOG_I("'Wi-Fi profile' at index %u saved to NVS", index);

cleanup:
    if (ret != ESP_OK) {
        EIF_LOG_E("Failed to save 'Wi-Fi profile' at index %u to NVS, err: %s",
                  index, esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t nvs_wifi_profile_load(
    uint8_t index,
    char (*ssid_out)[NVS_WIFI_SSID_MAX_LEN + 1U],
    char (*pass_out)[NVS_WIFI_PASS_MAX_LEN + 1U]
) {
    esp_err_t ret = ESP_OK;
    char ssid_key[WIFI_KEY_LEN + 1U] = {0};
    char pass_key[WIFI_KEY_LEN + 1U] = {0};

    EIF_CHECK_NOT_NULL(ssid_out, ESP_ERR_INVALID_ARG);
    EIF_CHECK_NOT_NULL(pass_out, ESP_ERR_INVALID_ARG);

    EIF_TRY(nvs_check_range(index, 0U, CONFIG_EIF_CUSTOM_WIFI_PROFILES_COUNT, true));

    if (index != 0U) {
        nvs_wifi_gen_keys(index, &ssid_key, &pass_key);
        EIF_TRY(nvs_value_load(ssid_key, *ssid_out, NVS_WIFI_SSID_MAX_LEN + 1U));
        EIF_TRY(nvs_value_load(pass_key, *pass_out, NVS_WIFI_PASS_MAX_LEN + 1U));
    } else {
        (void)memcpy(*ssid_out, NVS_WIFI_SSID_DEFAULT, sizeof(NVS_WIFI_SSID_DEFAULT));
        (void)memcpy(*pass_out, NVS_WIFI_PASS_DEFAULT, sizeof(NVS_WIFI_PASS_DEFAULT));
    }

    EIF_LOG_I("'Wi-Fi profile' at index %u loaded from NVS", index);

cleanup:
    if (ret != ESP_OK) {
        EIF_LOG_E("Failed to load 'Wi-Fi profile' at index %u from NVS, err: %s",
                  index, esp_err_to_name(ret));
    }
    return ret;
}

static inline esp_err_t nvs_initialize_wifi_profiles(void) {
    const char empty_ssid[NVS_WIFI_SSID_MAX_LEN + 1U] = {0};
    const char empty_pass[NVS_WIFI_PASS_MAX_LEN + 1U] = {0};

    esp_err_t ret = ESP_OK;
    char wifi_ssid[NVS_WIFI_SSID_MAX_LEN + 1U] = {0};
    char wifi_pass[NVS_WIFI_PASS_MAX_LEN + 1U] = {0};

    /* @note The Wi-Fi profile with index 0 is hard-coded in the code (read-only) and should not
     *       be overwritten or deleted.Thus, the valid user-accessible range starts from 1. */
    for (size_t index = 1U; index <= CONFIG_EIF_CUSTOM_WIFI_PROFILES_COUNT; index++) {
        if ((ret = nvs_wifi_profile_load(index, &wifi_ssid, &wifi_pass)) == ESP_ERR_NVS_NOT_FOUND) {
            EIF_LOG_W("'Wi-Fi profile' at index %u not found in NVS. Creating empty...",
                      (unsigned int)index);
            EIF_TRY(nvs_wifi_profile_save(index, &empty_ssid, &empty_pass));
        } else if (ret != ESP_OK) {
            break;
        }
    }

    if (ret == ESP_OK) {
        EIF_LOG_I("'Wi-Fi profile's initialization finished");
    }

cleanup:
    if (ret != ESP_OK) {
        EIF_LOG_E("'Wi-Fi profile's initialization failed, err: %s", esp_err_to_name(ret));
    }
    return ret;
}



/* Decryption:
 * - Login:    'admin'
 * - Password: ''      (yes, without password) */
#define NVS_BASIC_AUTH_LINE_DEFAULT     "Basic YWRtaW46"
#define NVS_BASIC_AUTH_LINE_DEFAULT_LEN sizeof(NVS_BASIC_AUTH_LINE_DEFAULT) - 1U
#define NVS_KEY_BASIC_AUTH_LINE         "eif_basic_auth"

esp_err_t nvs_basic_auth_line_save(const char (*pass)[NVS_BASIC_AUTH_PASS_MAX_LEN + 1U]) {
    esp_err_t ret = ESP_OK;
    size_t written_b64_bytes = 0U;
    char line[NVS_BASIC_AUTH_LINE_MAX_LEN + 1U] = {0};
    size_t avaliable_b64_space = sizeof(line) - NVS_BASIC_AUTH_LINE_DEFAULT_LEN;
        
    EIF_CHECK_NOT_NULL(pass, ESP_ERR_INVALID_ARG);
        
    size_t pass_len = strnlen((const char *)*pass, NVS_BASIC_AUTH_PASS_MAX_LEN);
    EIF_TRY(nvs_check_range(pass_len, NVS_BASIC_AUTH_PASS_MIN_LEN,
                            NVS_BASIC_AUTH_PASS_MAX_LEN, true));

    (void)memcpy(line, NVS_BASIC_AUTH_LINE_DEFAULT, NVS_BASIC_AUTH_LINE_DEFAULT_LEN);

    if (mbedtls_base64_encode(
        (unsigned char *)&line[NVS_BASIC_AUTH_LINE_DEFAULT_LEN], avaliable_b64_space,
        &written_b64_bytes, (const unsigned char *)*pass, pass_len
    ) != 0) {
        ret = ESP_ERR_INVALID_SIZE;
        goto cleanup;
    }

    EIF_TRY(nvs_value_save(NVS_KEY_BASIC_AUTH_LINE, line));

    EIF_LOG_I("'Basic Auth credentials' saved to NVS");
    (void)written_b64_bytes;

cleanup:
    if (ret != ESP_OK) {
        EIF_LOG_E("Failed to save 'Basic Auth credentials' to NVS, err: %s",
                  esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t nvs_basic_auth_line_load(char (*buf_out)[NVS_BASIC_AUTH_LINE_MAX_LEN + 1U]) {
    esp_err_t ret = ESP_OK;
        
    EIF_CHECK_NOT_NULL(buf_out, ESP_ERR_INVALID_ARG);
    EIF_TRY(nvs_value_load(NVS_KEY_BASIC_AUTH_LINE, (char * const)(*buf_out),
                           NVS_BASIC_AUTH_LINE_MAX_LEN + 1U));

    EIF_LOG_I("'Basic Auth credentials' loaded from NVS");

cleanup:
    if (ret != ESP_OK) {
        EIF_LOG_E("Failed to load 'Basic Auth credentials' from NVS, err: %s",
                  esp_err_to_name(ret));
    }
    return ret;
}

static inline esp_err_t nvs_initialize_basic_auth(void) {
    const char empty_auth_pass[NVS_BASIC_AUTH_PASS_MAX_LEN + 1U] = {0};

    esp_err_t ret = ESP_OK;
    char basic_auth_buffer[NVS_BASIC_AUTH_LINE_MAX_LEN + 1U] = {0};

    if ((ret = nvs_basic_auth_line_load(&basic_auth_buffer)) == ESP_ERR_NVS_NOT_FOUND) {
        EIF_LOG_W("'Basic Auth credentials' not found in NVS. Creating default...");
        EIF_TRY(nvs_basic_auth_line_save(&empty_auth_pass));
    }
    
    if (ret == ESP_OK) {
        EIF_LOG_I("Basic Auth initialization finished");
    }

cleanup:
    if (ret != ESP_OK) {
        EIF_LOG_E("'Basic Auth credentials' initialization failed, err: %s", esp_err_to_name(ret));
    }
    return ret;
}



#ifdef CONFIG_EIF_ENABLE_TLS
    #define NVS_KEY_TLS_CERT     "eif_tls_cert"
    #define NVS_KEY_TLS_PRIV_KEY "eif_tls_key"

    esp_err_t nvs_tls_creds_save(const char * const cert, const char * const priv_key) {
        esp_err_t ret = ESP_OK;
        
        EIF_CHECK_NOT_NULL(cert, ESP_ERR_INVALID_ARG);
        EIF_CHECK_NOT_NULL(priv_key, ESP_ERR_INVALID_ARG);

        EIF_TRY(nvs_value_save2(NVS_KEY_TLS_CERT, cert, NVS_KEY_TLS_PRIV_KEY, priv_key));

        EIF_LOG_I("'TLS credentials' saved to NVS");;

    cleanup:
        if (ret != ESP_OK) {
            EIF_LOG_E("Failed to save 'TLS credentials' to NVS, err: %s",
                      esp_err_to_name(ret));
        }
        return ret;
    }

    esp_err_t nvs_tls_creds_load(
        char ** const cert,     size_t * const cert_len,
        char ** const priv_key, size_t * const priv_key_len
    ) {
        esp_err_t ret = ESP_OK;
        
        EIF_CHECK_NOT_NULL(cert,         ESP_ERR_INVALID_ARG);
        EIF_CHECK_NOT_NULL(cert_len,     ESP_ERR_INVALID_ARG);
        EIF_CHECK_NOT_NULL(priv_key,     ESP_ERR_INVALID_ARG);
        EIF_CHECK_NOT_NULL(priv_key_len, ESP_ERR_INVALID_ARG);
        
        EIF_TRY(nvs_value_load_malloc(NVS_KEY_TLS_CERT,     cert,     cert_len));
        EIF_TRY(nvs_value_load_malloc(NVS_KEY_TLS_PRIV_KEY, priv_key, priv_key_len));

        EIF_LOG_I("'TLS credentials' loaded from NVS");
        EIF_LOG_I("Cert len: '%u', Private Key len: '%u'",
                  (unsigned int)*cert_len, (unsigned int)*priv_key_len);

    cleanup:
        if (ret != ESP_OK) {
            if (cert != NULL && *cert != NULL) {
                vPortFree(*cert);
                *cert_len = 0U;
                *cert = NULL;
            }
            if (priv_key != NULL && *priv_key != NULL) {
                vPortFree(*priv_key);
                *priv_key_len = 0U;
                *priv_key = NULL;
            }

            EIF_LOG_E("Failed to load 'TLS credentials' from NVS, err: %s",
                      esp_err_to_name(ret));
        }
        return ret;
    }
    
    static inline esp_err_t nvs_initialize_tls(void) {
        esp_err_t ret = ESP_OK;
        char *buf_cert = NULL;
        size_t buf_cert_len = 0U;
        char *buf_priv_key = NULL;
        size_t buf_priv_key_len = 0U;

        ret = nvs_tls_creds_load(&buf_cert, &buf_cert_len, &buf_priv_key, &buf_priv_key_len);
        if ((ret != ESP_OK) && (ret != ESP_ERR_NVS_NOT_FOUND)) {
            goto cleanup;
        }

        if (ret == ESP_ERR_NVS_NOT_FOUND) {
            EIF_LOG_W("'TLS credentials' not found in NVS. Creating new ones...");
            
            bootloader_random_enable();
            EIF_TRY(sys_generate_tls_credentials_sync());
            bootloader_random_disable();

            EIF_TRY(nvs_tls_creds_load(&buf_cert, &buf_cert_len, &buf_priv_key, &buf_priv_key_len));
        }
        
        config_set_tls_cred(buf_cert, buf_cert_len, buf_priv_key, buf_priv_key_len);

        EIF_LOG_I("'TLS credentials' initialization finished");
    cleanup:
        if (buf_cert != NULL) {
            vPortFree(buf_cert);
        }
        if (buf_priv_key != NULL) {
            vPortFree(buf_priv_key);
        }
        if (ret != ESP_OK) {
            EIF_LOG_E("'TLS credentials' initialization failed, err: %s", esp_err_to_name(ret));
        }
        return ret;
    }
#endif



/* --- */

esp_err_t nvs_initialize(void) {
    esp_err_t ret = ESP_OK;
    
    ret = nvs_flash_init();
    if ((ret == ESP_ERR_NVS_NO_FREE_PAGES) || (ret == ESP_ERR_NVS_NEW_VERSION_FOUND)) {
        EIF_LOG_W("NVS flash storage is full or has a new version. Erasing and retrying...");
         
        EIF_TRY(nvs_flash_erase());
        EIF_TRY(nvs_flash_init());

        ret = ESP_OK;
    } else if (ret != ESP_OK) {
        goto cleanup;
    }

    EIF_TRY(nvs_initialize_wifi_profiles());
    EIF_TRY(nvs_initialize_basic_auth());
    #ifdef CONFIG_EIF_ENABLE_TLS
        EIF_TRY(nvs_initialize_tls());
    #endif

    EIF_LOG_I("'NVS subsystem' initialization finished");

cleanup:
    if (ret != ESP_OK) {
        EIF_LOG_E("'NVS subsystem' initialization failed, err: %s", esp_err_to_name(ret));
    }
    return ret;
}