/*
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp_iot_framework
 * Folder: src
 * File: nvs.c
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

#include "nvs.h"
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"

#include "macros.h"
#include "core_internal.h"
#include "esp_iot_framework.h"

#define _NAME "core"

/* --- */

#define _ERR_OPEN_WRITE     "Can't open NVS for writing '%s'"
#define _ERR_OPEN_READ      "Can't open NVS for reading '%s'"
#define _ERR_WRITE          "Writing '%s' failed"
#define _ERR_READ           "Reading '%s' failed"
#define _ERR_COMMIT         "Failed to commit '%s'"
#define _ERR_MISSING        "'%s' is missing"
#define _ERR_ALLOCATE       "Failed to allocate %d bytes for '%s'"
#define _ERR_SAVING         "Failed to save"
#define _ERR_INVALID_IDX     "Invalid '%s' index: %u (range: %u-%u)"
#define _ERR_INVALID_LEN     "Invalid '%s' length: %u (range: %u-%u)"

#define _WARN_USING_DEF     "Using default for `%s`"
#define _WARN_USING_DEF_IDX "Using default for `%s #%zu`"

#define _MSG_SAVE_OK        "`%s` saved successfully"
#define _MSG_SAVE_OK_IDX    "`%s #%d` saved successfully"
#define _MSG_LOAD_OK        "`%s` loaded successfully"
#define _MSG_LOAD_OK_IDX    "`%s #%d` loaded successfully"
#define _MSG_RECREATION     "`%s` re-creation launches"

#ifdef CONFIG_EIF_ENABLE_TLS
    #define _ERR_GEN_CERT   "MbedTLS cert generation failed"
    #define _MSG_RECREAT_OK "Re-creation of `%s` completed successfully"
#endif

/* --- */

#define TAG "NVS WiFi"
#define _OBJ_NAME "WiFi profile"
#define _OBJ_WIFI_PROF "WiFi profile"

// "wifi_ssid" (9) + max index "255" (3) + \0 (1) = 13
#define WIFI_KEY_LEN 13

static void nvs_wifi_gen_keys(
    uint8_t index, char *s_buf, char *p_buf
) {
    snprintf(s_buf, WIFI_KEY_LEN, "wifi_ssid%u", index);
    snprintf(p_buf, WIFI_KEY_LEN, "wifi_pass%u", index);
}

esp_err_t nvs_wifi_profile_save(
    uint8_t index, const char* ssid, const char* pass
) {
    esp_err_t ret = ESP_OK;
    CHECK_NOT_NULL(ssid, ESP_ERR_INVALID_ARG, return ret);
    CHECK_NOT_NULL(pass, ESP_ERR_INVALID_ARG, return ret);

    const eif_t *cfg = eif_get();

    if (index < 1 || index > cfg->wifi_profiles_count) {
        CORE_LOG(E, _ERR_INVALID_IDX, 
            _OBJ_NAME, index, 1, cfg->wifi_profiles_count);
        return ESP_ERR_INVALID_ARG;
    }

    char ssid_key[WIFI_KEY_LEN], pass_key[WIFI_KEY_LEN];
    nvs_wifi_gen_keys(index, ssid_key, pass_key);
    size_t ssid_len = strlen(ssid), pass_len = strlen(pass);

    if (!(ssid_len == 0 && pass_len == 0)) {
        if (ssid_len < SSID_MIN_LEN || ssid_len >= SSID_MAX_LEN) {
            CORE_LOG(E, _ERR_INVALID_LEN, 
                ssid_key, ssid_len, SSID_MIN_LEN, SSID_MAX_LEN - 1);
            return ESP_ERR_INVALID_SIZE;
        }
        if (pass_len < PASSWORD_MIN_LEN || pass_len >= PASSWORD_MAX_LEN) {
            CORE_LOG(E, _ERR_INVALID_LEN,
                pass_key, pass_len, PASSWORD_MIN_LEN, PASSWORD_MAX_LEN - 1);
            return ESP_ERR_INVALID_SIZE;
        }
    }
    
    nvs_handle_t handle;
    CHECK_ESP_ERR_T(E, nvs_open(_NAME, NVS_READWRITE, &handle), 
        {}, GOTO_CLEANUP_ERR(), _ERR_OPEN_WRITE, _OBJ_NAME);

    CHECK_ESP_ERR_T(E, nvs_set_str(handle, ssid_key, ssid), 
        {}, GOTO_CLEANUP_ERR(), _ERR_WRITE, ssid_key);

    CHECK_ESP_ERR_T(E, nvs_set_str(handle, pass_key, pass), 
        {}, GOTO_CLEANUP_ERR(), _ERR_WRITE, pass_key);
    
    CHECK_ESP_ERR_T(E, nvs_commit(handle), 
        {}, GOTO_CLEANUP_ERR(), _ERR_COMMIT, _OBJ_NAME);

    CORE_LOG(I, _MSG_SAVE_OK_IDX, _OBJ_NAME, index);
cleanup:
    nvs_close(handle);
    return ret;
}

esp_err_t nvs_wifi_profile_load(
    uint8_t index, char* ssid_out, char* pass_out
) {
    esp_err_t ret = ESP_OK;
    CHECK_NOT_NULL(ssid_out, ESP_ERR_INVALID_ARG, return ret);
    CHECK_NOT_NULL(pass_out, ESP_ERR_INVALID_ARG, return ret);

    const eif_t *cfg = eif_get();

    if (index > cfg->wifi_profiles_count) {
        CORE_LOG(E, _ERR_INVALID_IDX, 
            _OBJ_NAME, index, 0, cfg->wifi_profiles_count);
        return ESP_ERR_INVALID_ARG;
    }

    if (index == 0) {
        snprintf(ssid_out, SSID_MAX_LEN, "%s", WIFI_DEFAULT_SSID);
        snprintf(pass_out, PASSWORD_MAX_LEN, "%s", WIFI_DEFAULT_PASS);
        CORE_LOG(I, _WARN_USING_DEF_IDX, _OBJ_NAME, 0);
        return ESP_OK;
    }

    nvs_handle_t handle;
    CHECK_ESP_ERR_T(E, nvs_open(_NAME, NVS_READONLY, &handle), 
        {}, return err, _ERR_OPEN_READ, _OBJ_NAME);

    char ssid_key[WIFI_KEY_LEN], pass_key[WIFI_KEY_LEN];
    nvs_wifi_gen_keys(index, ssid_key, pass_key);

    size_t s_limit = SSID_MAX_LEN;
    size_t p_limit = PASSWORD_MAX_LEN;

    CHECK_ESP_ERR_T(E, nvs_get_str(handle, ssid_key, ssid_out, &s_limit), 
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            CORE_LOG(W, _ERR_MISSING, ssid_key);
            ssid_out[0] = '\0';
            break;
        }, GOTO_CLEANUP_ERR(), _ERR_READ, ssid_key);

    CHECK_ESP_ERR_T(E, nvs_get_str(handle, pass_key, pass_out, &p_limit), 
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            CORE_LOG(W, _ERR_MISSING, pass_key);
            pass_out[0] = '\0';
            break;
        }, GOTO_CLEANUP_ERR(), _ERR_READ, pass_key);

    CORE_LOG(I, _MSG_LOAD_OK_IDX, _OBJ_NAME, index);
cleanup:
    nvs_close(handle);
    return ret;
}

#undef TAG
#undef _OBJ_NAME

/* --- */

#ifdef CONFIG_EIF_ENABLE_TLS
    #define TAG "NVS TLS"
    #define _OBJ_NAME "TLS credentials"
    #define _OBJ_TLS_CREDS "TLS credentials"

    #define _F_CACERT_KEY "tls_certificate"
    #define _F_PRVTKEY_KEY "tls_private_key"

    static esp_err_t nvs_tls_creds_save(
        const char* cert_pem, const char* key_pem
    ) {
        esp_err_t ret = ESP_OK;
        CHECK_NOT_NULL(cert_pem, ESP_ERR_INVALID_ARG, return ret);
        CHECK_NOT_NULL(key_pem, ESP_ERR_INVALID_ARG, return ret);

        nvs_handle_t handle;
        CHECK_ESP_ERR_T(E, nvs_open(_NAME, NVS_READWRITE, &handle), 
            {}, GOTO_CLEANUP_ERR(), _ERR_OPEN_WRITE, _OBJ_NAME);

        CHECK_ESP_ERR_T(E, nvs_set_str(handle, _F_CACERT_KEY, cert_pem), 
            {}, GOTO_CLEANUP_ERR(), _ERR_WRITE, _F_CACERT_KEY);
            
        CHECK_ESP_ERR_T(E, nvs_set_str(handle, _F_PRVTKEY_KEY, key_pem), 
            {}, GOTO_CLEANUP_ERR(), _ERR_WRITE, _F_PRVTKEY_KEY);
        
        CHECK_ESP_ERR_T(E, nvs_commit(handle), 
            {}, GOTO_CLEANUP_ERR(), _ERR_COMMIT, _OBJ_NAME);

        CORE_LOG(I, _MSG_SAVE_OK, _OBJ_NAME);
    cleanup:
        nvs_close(handle);
        return ret;
    }

    esp_err_t nvs_tls_creds_load(
        char** cert_out, size_t* cert_out_len, 
        char** key_out, size_t* key_out_len
    ) {
        esp_err_t ret = ESP_OK;
        CHECK_NOT_NULL(cert_out, ESP_ERR_INVALID_ARG, return ret);
        CHECK_NOT_NULL(cert_out_len, ESP_ERR_INVALID_ARG, return ret);
        CHECK_NOT_NULL(key_out, ESP_ERR_INVALID_ARG, return ret);
        CHECK_NOT_NULL(key_out_len, ESP_ERR_INVALID_ARG, return ret);

        *cert_out = NULL; *key_out = NULL;
        *cert_out_len = 0; *key_out_len = 0;

        nvs_handle_t handle;
        CHECK_ESP_ERR_T(E, nvs_open(_NAME, NVS_READONLY, &handle), 
            {}, GOTO_CLEANUP_ERR(), _ERR_OPEN_READ, _OBJ_NAME);

        size_t cert_len = 0, key_len = 0;
        CHECK_ESP_ERR_T(E, nvs_get_str(
            handle, _F_CACERT_KEY, NULL, &cert_len
        ), {}, GOTO_CLEANUP_ERR(), _ERR_MISSING, _F_CACERT_KEY);
            
        CHECK_ESP_ERR_T(E, nvs_get_str(
            handle, _F_PRVTKEY_KEY, NULL, &key_len
        ), {}, GOTO_CLEANUP_ERR(), _ERR_MISSING, _F_PRVTKEY_KEY);

        *cert_out = malloc(cert_len);
        *key_out = malloc(key_len);

        if (!*cert_out || !*key_out) {
            CORE_LOG(E, _ERR_ALLOCATE, cert_len + key_len, _OBJ_NAME);
            ret = ESP_ERR_NO_MEM;
            goto cleanup;
        }

        CHECK_ESP_ERR_T(E, nvs_get_str(
            handle, _F_CACERT_KEY, *cert_out, &cert_len
        ), {}, GOTO_CLEANUP_ERR(), _ERR_READ, _F_CACERT_KEY);
        
        CHECK_ESP_ERR_T(E, nvs_get_str(
            handle, _F_PRVTKEY_KEY, *key_out, &key_len
        ), {}, GOTO_CLEANUP_ERR(), _ERR_READ, _F_PRVTKEY_KEY);

        *cert_out_len = cert_len; 
        *key_out_len = key_len;

        CORE_LOG(I, _MSG_LOAD_OK, _OBJ_NAME);

        nvs_close(handle);
        return ESP_OK;
    cleanup:
        if (*cert_out) free(*cert_out);
        if (*key_out) free(*key_out);
        *cert_out = NULL; *key_out = NULL;
        nvs_close(handle);
        return ret;
    }

    esp_err_t nvs_tls_creds_create_and_save(void) {
        esp_err_t ret = ESP_OK;
        char *cert_out = NULL, *key_out = NULL;
        size_t cert_len = 0, key_len = 0;

        CHECK_ESP_ERR_T(E, generate_https_certs(
            &cert_out, &cert_len, &key_out, &key_len
        ), {}, GOTO_CLEANUP_ERR(), _ERR_GEN_CERT);

        CHECK_ESP_ERR_T(E, nvs_tls_creds_save(cert_out, key_out), {}, 
            GOTO_CLEANUP_ERR(), _ERR_SAVING);

        CORE_LOG(I, _MSG_RECREAT_OK, _OBJ_NAME);
    cleanup:
        free(cert_out); free(key_out);
        return ret;
    }

    #undef TAG
    #undef _OBJ_NAME
#endif

/* --- */

#ifdef CONFIG_EIF_ENABLE_BASIC_AUTH
    #define TAG "NVS Auth Data"
    #define _OBJ_NAME "Authorization data"
    #define _OBJ_AUTH_DATA "Authorization data"

    esp_err_t nvs_auth_line_save(const char* pass) {
        esp_err_t ret = ESP_OK;
        nvs_handle_t handle;

        CHECK_NOT_NULL(pass, ESP_ERR_INVALID_ARG, return ret);

        size_t len = strlen(pass);
        if (len < AUTH_LINE_MIN_LEN || len >= AUTH_LINE_MAX_LEN) {
            CORE_LOG(E, _ERR_INVALID_LEN, 
                _F_AUTH_LINE, len, AUTH_LINE_MIN_LEN, AUTH_LINE_MAX_LEN - 1);
            return ESP_ERR_INVALID_SIZE;
        }

        CHECK_ESP_ERR_T(E, nvs_open(_NAME, NVS_READWRITE, &handle), 
            {}, return err, _ERR_OPEN_WRITE, _OBJ_NAME);
        CHECK_ESP_ERR_T(E, nvs_set_str(handle, _F_AUTH_LINE, pass), 
            {}, GOTO_CLEANUP_ERR(), _ERR_WRITE, _F_AUTH_LINE);
        CHECK_ESP_ERR_T(E, nvs_commit(handle), 
            {}, GOTO_CLEANUP_ERR(), _ERR_COMMIT, _OBJ_NAME);

        CORE_LOG(I, _MSG_SAVE_OK, _OBJ_NAME);
    cleanup:
        nvs_close(handle);
        return ret;
    }

    esp_err_t nvs_auth_line_load(char* out_pass) {
        esp_err_t ret = ESP_OK;
        nvs_handle_t handle;

        CHECK_NOT_NULL(out_pass, ESP_ERR_INVALID_ARG, return ret);

        CHECK_ESP_ERR_T(E, nvs_open(_NAME, NVS_READONLY, &handle), 
            {}, return err, _ERR_OPEN_READ, _OBJ_NAME);

        size_t required_size = AUTH_LINE_MAX_LEN;
        CHECK_ESP_ERR_T(E, nvs_get_str(
            handle, _F_AUTH_LINE, out_pass, &required_size
        ), {}, GOTO_CLEANUP_ERR(), _ERR_READ, _F_AUTH_LINE);

        CORE_LOG(I, _MSG_LOAD_OK, _OBJ_NAME);
    cleanup:
        nvs_close(handle);
        return ret;
    }

    #undef TAG
    #undef _OBJ_NAME
#endif

/* --- */

#define TAG "NVS Main"

#define _OBJ_FLASH_INIT   "Flash Init"
#define _OBJ_FLASH_REINIT "Flash Re-init"
#define _OBJ_FLASH_ERASE  "Flash Erase"

esp_err_t eif_nvs_initialize(void) {
    CHECK_ESP_ERR_T(E, nvs_flash_init(), 
        if (err == ESP_ERR_NVS_NO_FREE_PAGES 
            || err == ESP_ERR_NVS_NEW_VERSION_FOUND
        ) {
            CORE_LOG(W, _MSG_RECREATION, _OBJ_FLASH_INIT);
            CHECK_ESP_ERR_T(E, nvs_flash_erase(), 
                {}, return err, _ERR_OPEN_WRITE, _OBJ_FLASH_ERASE);
            CHECK_ESP_ERR_T(E, nvs_flash_init(), 
                {}, return err, _ERR_OPEN_WRITE, _OBJ_FLASH_REINIT);
        }, return err, _ERR_OPEN_WRITE, _OBJ_FLASH_INIT);
    
    CORE_LOG(I, "NVS Flash system is ready");
    const eif_t* cfg = eif_get();

    for (size_t i = 1; i <= cfg->wifi_profiles_count; i++) {
        char ssid[SSID_MAX_LEN] = {0}, pass[PASSWORD_MAX_LEN] = {0};
        CHECK_ESP_ERR_T(W, nvs_wifi_profile_load(i, ssid, pass), {}, {
            CORE_LOG(I, _WARN_USING_DEF_IDX, _OBJ_WIFI_PROF, i);
            CHECK_ESP_ERR_T(E, nvs_wifi_profile_save(
                i, WIFI_EMPTY_SSID, WIFI_EMPTY_PASS
            ), {}, return err, _ERR_SAVING);
        }, _ERR_MISSING, _OBJ_WIFI_PROF);
    }

    #ifdef CONFIG_EIF_ENABLE_TLS
        char   *cert_out = NULL, *key_out = NULL;
        size_t  cert_len = 0,     key_len = 0;
        CHECK_ESP_ERR_T(W, nvs_tls_creds_load(
            &cert_out, &cert_len, &key_out, &key_len
        ), { free(cert_out); free(key_out); }, 
        {
            CORE_LOG(I, _MSG_RECREATION, _OBJ_TLS_CREDS);
            CHECK_ESP_ERR_T(E, nvs_tls_creds_create_and_save(), 
                {}, return err, _ERR_GEN_CERT);
        }, _ERR_MISSING, _OBJ_TLS_CREDS);
    #endif

    #ifdef CONFIG_EIF_ENABLE_BASIC_AUTH
        char tmp_pw[AUTH_LINE_MAX_LEN];
        CHECK_ESP_ERR_T(W, nvs_auth_line_load(tmp_pw), {}, {
            CORE_LOG(I, _WARN_USING_DEF, _OBJ_AUTH_DATA);
            CHECK_ESP_ERR_T(E, nvs_auth_line_save(AUTH_LINE_DEF), 
                {}, return err, _ERR_SAVING);
        }, _ERR_MISSING, _OBJ_AUTH_DATA);
    #endif

    CORE_LOG(I, "All NVS fields initialized successfully");
    return ESP_OK;
}