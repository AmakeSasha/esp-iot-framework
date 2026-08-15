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
#include <esp_log.h>
#include <esp_err.h>
#include <nvs_flash.h>

#include "core_internal.h"
#include "esp_iot_framework_core.h"
#include "esp_iot_framework_core_macros.h"

#define NVS_STORAGE_NAME "_eif_nvs_core_"

/* --- */

#define NVS_ERR_OPEN_WRITE  "Can't open NVS for writing '%s'"
#define NVS_ERR_OPEN_READ   "Can't open NVS for reading '%s'"
#define NVS_ERR_SAVE        "Saving '%s' failed"
#define NVS_ERR_SAVE_F_IDX  "Saving Wi-Fi profile field '%s \x23" "%d' failed"
#define NVS_ERR_LOAD        "Loading '%s' failed"
#define NVS_ERR_LOAD_F_IDX  "Loading Wi-Fi profile field '%s \x23" "%d' failed"
#define NVS_ERR_MISSING     "'%s' is missing"
#define NVS_ERR_MISSING_IDX "'%s \x23" "%d' is missing"
#define NVS_ERR_GEN_KEYS    "Error in formatting keys for the Wi-Fi profile \x23" "%d"
#define NVS_ERR_INVALID_IDX "Invalid '%s' index: %u (range: %u-%u)"
#define NVS_ERR_INVALID_LEN "Invalid '%s' length: %u (range: %u-%u)"
#define NVS_ERR_COMMIT      "Failed to commit '%s'"
#define NVS_ERR_ALLOCATE    "Failed to allocate %d bytes for '%s'"

#define NVS_MSG_SAVE_OK        "'%s' saved successfully"
#define NVS_MSG_SAVE_OK_IDX    "'%s \x23" "%d' saved successfully"
#define NVS_MSG_LOAD_OK        "'%s' loaded successfully"
#define NVS_MSG_LOAD_OK_IDX    "'%s \x23" "%d' loaded successfully"
#define NVS_MSG_LOAD_OK_MALLOC "'%s' with len '%d' loaded successfully"

#define EIF_NVS_INVALID_HANDLE ((nvs_handle_t)0U)

/* --- */

#define TAG "NVS"

static inline esp_err_t eif_nvs_check_range(
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

esp_err_t eif_nvs_value_save(
    const char * const key, const char * const value,
    size_t min_len, size_t max_len, bool it_can_be_empty
) {
    esp_err_t ret = ESP_OK;
    nvs_handle_t handle = EIF_NVS_INVALID_HANDLE;
    size_t value_len = 0U;

    EIF_IF_OK_CHECK_NOT_NULL(ret, key, ESP_ERR_INVALID_ARG);
    EIF_IF_OK_CHECK_NOT_NULL(ret, value, ESP_ERR_INVALID_ARG);

    if (ret == ESP_OK) {
        value_len = eif_strnlen(value, max_len);
    }
    /* @note The macro includes the null-terminator, so 1 is subtracted
     * to get the correct character count for the length check. */
    EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_nvs_check_range(
        value_len, min_len, max_len - 1U, it_can_be_empty
    ), NVS_ERR_INVALID_LEN, key, value_len, min_len, max_len);

    EIF_IF_OK_CHECK_ESP_ERR_T(ret,
        nvs_open(NVS_STORAGE_NAME, NVS_READWRITE, &handle),
        NVS_ERR_OPEN_WRITE, key);
    EIF_IF_OK_CHECK_ESP_ERR_T(ret, nvs_set_str(handle, key, value),
        NVS_ERR_SAVE, key);
    EIF_IF_OK_CHECK_ESP_ERR_T(ret, nvs_commit(handle),
        NVS_ERR_COMMIT, key);

    if (ret == ESP_OK) {
        EIF_LOG_I(NVS_MSG_SAVE_OK, key);
    }

    /* Cleanup */
    if (handle != EIF_NVS_INVALID_HANDLE) {
        (void)nvs_close(handle);
    }
    return ret;
}

esp_err_t eif_nvs_value_load(
    const char * const key, char * const value_out, size_t max_len
) {
    esp_err_t ret = ESP_OK;
    nvs_handle_t handle = EIF_NVS_INVALID_HANDLE;

    EIF_IF_OK_CHECK_NOT_NULL(ret, key, ESP_ERR_INVALID_ARG);
    EIF_IF_OK_CHECK_NOT_NULL(ret, value_out, ESP_ERR_INVALID_ARG);

    EIF_IF_OK_CHECK_ESP_ERR_T(ret,
        nvs_open(NVS_STORAGE_NAME, NVS_READWRITE, &handle),
        NVS_ERR_OPEN_READ, key);

    size_t length = max_len;
    EIF_IF_OK_CHECK_ESP_ERR_T(ret, nvs_get_str(handle, key, value_out, &length),
        NVS_ERR_LOAD, key);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        EIF_LOG_W(NVS_ERR_MISSING, key);
        if (max_len > 1U) {
            value_out[0] = '\0';
        }
    }

    if (ret == ESP_OK) {
        EIF_LOG_I(NVS_MSG_LOAD_OK, key);
    }

    /* Cleanup */
    if (handle != EIF_NVS_INVALID_HANDLE) {
        (void)nvs_close(handle);
    }
    return ret;
}

esp_err_t eif_nvs_value_load_malloc(
    const char * key, char ** const value_out, size_t * const value_out_len
) {
    esp_err_t ret = ESP_OK;
    nvs_handle_t handle = EIF_NVS_INVALID_HANDLE;
    size_t length = 0U;

    EIF_IF_OK_CHECK_NOT_NULL(ret, key, ESP_ERR_INVALID_ARG);
    EIF_IF_OK_CHECK_NOT_NULL(ret, value_out, ESP_ERR_INVALID_ARG);
    EIF_IF_OK_CHECK_NOT_NULL(ret, value_out_len, ESP_ERR_INVALID_ARG);

    if ((ret == ESP_OK) && (value_out)) {
        if (*value_out != NULL) {
            vPortFree(*value_out);
        }
        *value_out = NULL;
    }

    EIF_IF_OK_CHECK_ESP_ERR_T(ret,
        nvs_open(NVS_STORAGE_NAME, NVS_READWRITE, &handle),
        NVS_ERR_OPEN_READ, key);
    EIF_IF_OK_CHECK_ESP_ERR_T(ret, nvs_get_str(handle, key, NULL, &length),
        NVS_ERR_MISSING, key);

    if (ret == ESP_OK) {
        *value_out = (char *)pvPortMalloc(length);
        EIF_IF_OK_CHECK_NOT_NULL(ret, *value_out, ESP_ERR_NO_MEM);
        if (ret == ESP_ERR_NO_MEM) {
            EIF_LOG_E(NVS_ERR_ALLOCATE, length, key);
        }
    }

    EIF_IF_OK_CHECK_ESP_ERR_T(ret,
        nvs_get_str(handle, key, *value_out, &length),
        NVS_ERR_LOAD, key);
    if (ret == ESP_OK) {
        *value_out_len = length;
        EIF_LOG_I(NVS_MSG_LOAD_OK_MALLOC, key, length);
    }

    /* Cleanup */
    if (handle != EIF_NVS_INVALID_HANDLE) {
        (void)nvs_close(handle);
    }
    if (ret != ESP_OK) {
        if (*value_out != NULL) {
            vPortFree(*value_out);
        }
        *value_out = NULL;
        *value_out_len = 0U;
    }
    return ret;
}

/* --- */
/* "wifi_ssid" (9) + max index "FF" (2) + \0 (1) = 12 */
#define WIFI_KEY_LEN (12U)

static inline esp_err_t eif_nvs_wifi_gen_keys(
    uint8_t index, char * const s_buf, char * const p_buf
) {
    esp_err_t ret = ESP_OK;

    const char hex_chars[] = "0123456789ABCDEF";

    EIF_IF_OK_CHECK_NOT_NULL(ret, s_buf, ESP_ERR_INVALID_ARG);
    EIF_IF_OK_CHECK_NOT_NULL(ret, p_buf, ESP_ERR_INVALID_ARG);

    if (ret == ESP_OK) {
        (void)memcpy(s_buf, "wifi_ssid", 9U);
        (void)memcpy(p_buf, "wifi_pass", 9U);

        size_t high_nibble = ((size_t)index >> 4U) & 0x0FU;
        size_t low_nibble = (size_t)index & 0x0FU;

        s_buf[9]  = hex_chars[high_nibble];
        s_buf[10] = hex_chars[low_nibble];
        s_buf[11] = '\0';

        p_buf[9]  = hex_chars[high_nibble];
        p_buf[10] = hex_chars[low_nibble];
        p_buf[11] = '\0';
    }

    /* Cleanup */
    return ret;
}

esp_err_t eif_nvs_wifi_profile_save(
    uint8_t index, const char * const ssid, const char * const pass
) {
    esp_err_t ret = ESP_OK;
    char ssid_key[WIFI_KEY_LEN] = {0};
    char pass_key[WIFI_KEY_LEN] = {0};
    bool it_can_be_empty = false;

    EIF_IF_OK_CHECK_NOT_NULL(ret, ssid, ESP_ERR_INVALID_ARG);
    EIF_IF_OK_CHECK_NOT_NULL(ret, pass, ESP_ERR_INVALID_ARG);

    /* @note The Wi-Fi profile with index 0 is hard-coded in the code
     * (read-only) and should not be overwritten or deleted.Thus, the
     * valid user-accessible range starts from 1. */
    EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_nvs_check_range(
        index, 1U, EIF_WIFI_PROFILES_MAX_COUNT, false
    ), NVS_ERR_INVALID_IDX, "wifi_index", index, 1U, EIF_WIFI_PROFILES_MAX_COUNT);

    EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_nvs_wifi_gen_keys(index,
        ssid_key, pass_key), NVS_ERR_GEN_KEYS, index);

    if (ret == ESP_OK) {
        it_can_be_empty = (eif_strnlen(ssid, EIF_WIFI_SSID_MAX_LEN) == 0U)
            && (eif_strnlen(pass, EIF_WIFI_PASS_MAX_LEN) == 0U);
    }

    /* @note Constants with the '_MAX_LEN' suffix define the total buffer
     * size in bytes, including the null terminator ('\0'). Since 'strnlen'
     * (used to check for going out of range.) counts characters excluding
     * the terminator, a subtraction of 1 is applied to establish the maximum
     * valid string length for range validation. */
    EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_nvs_value_save(ssid_key,
        ssid, EIF_WIFI_SSID_MIN_LEN, EIF_WIFI_SSID_MAX_LEN, it_can_be_empty
    ), NVS_ERR_SAVE_F_IDX, ssid_key, index);
    EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_nvs_value_save(pass_key,
        pass, EIF_WIFI_PASS_MIN_LEN, EIF_WIFI_PASS_MAX_LEN, it_can_be_empty
    ), NVS_ERR_SAVE_F_IDX, pass_key, index);

    if (ret == ESP_OK) {
        EIF_LOG_I(NVS_MSG_SAVE_OK_IDX, "WiFi profile", index);
    }

    /* Cleanup */
    return ret;
}

esp_err_t eif_nvs_wifi_profile_load(
    uint8_t index, char * const ssid_out, char * const pass_out
) {
    esp_err_t ret = ESP_OK;
    char ssid_key[WIFI_KEY_LEN] = {0};
    char pass_key[WIFI_KEY_LEN] = {0};

    EIF_IF_OK_CHECK_NOT_NULL(ret, ssid_out, ESP_ERR_INVALID_ARG);
    EIF_IF_OK_CHECK_NOT_NULL(ret, pass_out, ESP_ERR_INVALID_ARG);

    EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_nvs_check_range(
        index, 0U, EIF_WIFI_PROFILES_MAX_COUNT, true
    ), NVS_ERR_INVALID_IDX, "wifi_index", index, 0U, EIF_WIFI_PROFILES_MAX_COUNT);

    if ((ret == ESP_OK) && (index == 0U)) {
        size_t const s_len = eif_strnlen(EIF_WIFI_SSID_DEFAULT, EIF_WIFI_SSID_MAX_LEN);
        (void)memcpy(ssid_out, EIF_WIFI_SSID_DEFAULT, s_len);
        ssid_out[s_len] = '\0';

        size_t const p_len = eif_strnlen(EIF_WIFI_PASS_DEFAULT, EIF_WIFI_PASS_MAX_LEN);
        (void)memcpy(pass_out, EIF_WIFI_PASS_DEFAULT, p_len);
        pass_out[p_len] = '\0';
    } else {
        EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_nvs_wifi_gen_keys(index,
            ssid_key, pass_key), NVS_ERR_GEN_KEYS, index);
        EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_nvs_value_load(ssid_key, ssid_out,
            EIF_WIFI_SSID_MAX_LEN), NVS_ERR_LOAD_F_IDX, ssid_key, index);
        EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_nvs_value_load(pass_key, pass_out,
            EIF_WIFI_PASS_MAX_LEN), NVS_ERR_LOAD_F_IDX, pass_key, index);
    }

    if (ret == ESP_OK) {
        EIF_LOG_I(NVS_MSG_LOAD_OK_IDX, "WiFi profile", index);
    }

    /* Cleanup */
    return ret;
}

#ifdef CONFIG_EIF_ENABLE_BASIC_AUTH
    /* Decryption:
     * - Login:    'admin'
     * - Password: ''      (yes, without password) */
    #define EIF_BASIC_AUTH_LINE_DEFAULT "Basic YWRtaW46"
    #define NVS_KEY_BASIC_AUTH_LINE "web_auth_pass"

    static inline uint32_t read_next_octet(
        const unsigned char *input, size_t input_length, size_t *index
    ) {
        uint32_t octet = 0U;
        if ((*index) < input_length) {
            octet = (uint32_t)input[*index];
            (*index)++;
        }
        return octet;
    }
    
    static char *base64_encode(const unsigned char *input, size_t input_length) {
        static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
        esp_err_t err = ESP_OK;

        size_t output_length = 4U * ((input_length + 2U) / 3U);
        char *encoded_data = (char *)pvPortMalloc(output_length + 1U);
        EIF_IF_OK_CHECK_NOT_NULL(err, encoded_data, ESP_ERR_NO_MEM);

        if ((err == ESP_OK) && (encoded_data)) {
            size_t i = 0;
            size_t j = 0;

            while (i < input_length) {
                uint32_t octet_a = read_next_octet(input, input_length, &i);
                uint32_t octet_b = read_next_octet(input, input_length, &i);
                uint32_t octet_c = read_next_octet(input, input_length, &i);

                uint32_t triple = (octet_a << 16U) | (octet_b << 8U) | octet_c;

                encoded_data[j + 0U] = base64_table[(triple >> 18U) & 0x3FU];
                encoded_data[j + 1U] = base64_table[(triple >> 12U) & 0x3FU];
                encoded_data[j + 2U] = base64_table[(triple >> 6U) & 0x3FU];
                encoded_data[j + 3U] = base64_table[triple & 0x3FU];
                j += 4U;
            }

            const size_t padding_count = (3U - (input_length % 3U)) % 3U;
            for (size_t k = 0U; k < padding_count; k++) {
                encoded_data[output_length - 1U - k] = '=';
            }

            encoded_data[output_length] = '\0';
        }

        return (err == ESP_OK) ? encoded_data : NULL;
    }

    esp_err_t eif_nvs_basic_auth_line_save(const unsigned char * const pass) {
        esp_err_t ret = ESP_OK;
        char line[EIF_BASIC_AUTH_LINE_MAX_LEN] = {0};
        char *b64_pass = NULL;
        size_t pass_len = eif_strnlen(
            (const char *)pass, EIF_BASIC_AUTH_PASS_MAX_LEN - 1U
        );

        EIF_IF_OK_CHECK_NOT_NULL(ret, pass, ESP_ERR_INVALID_ARG);
        /* @note The macro includes the null-terminator, so 1 is subtracted
         * to get the correct character count for the length check. */
        EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_nvs_check_range(pass_len,
            EIF_BASIC_AUTH_PASS_MIN_LEN, EIF_BASIC_AUTH_PASS_MAX_LEN - 1U, true
        ), NVS_ERR_INVALID_LEN, "Basic Auth Password", pass_len,
            EIF_BASIC_AUTH_PASS_MIN_LEN, EIF_BASIC_AUTH_PASS_MAX_LEN);

        if (ret == ESP_OK) {
            b64_pass = base64_encode(pass, pass_len);
            EIF_IF_OK_CHECK_NOT_NULL(ret, b64_pass, ESP_ERR_INVALID_SIZE);
        }

        /* @note The 'admin:' credentials string is exactly 6 bytes. In Base64,
         * 6 bytes encode perfectly into 8 characters ('YWRtaW46') without any
         * '=' padding. This allows direct concatenation of the encoded password
         * to the default prefix, avoiding additional heap allocations for a
         * combined 'user:pass' buffer. */
        if (ret == ESP_OK) {
            /* @deviation [Rule 21.6] The use of 'snprintf' is justified as the
             * format string is constant and the inputs are strictly
             * length-validated. Buffer safety is guaranteed by passing
             * 'EIF_BASIC_AUTH_LINE_MAX_LEN' as the size limit and explicitly
             * checking the return value to ensure the output is not truncated.
             * This approach prevents malformed data from being written to NVS
             * and is more maintainable than manual string concatenation. */
            /* cppcheck-suppress misra-c2012-21.6 */
            int res = snprintf(line, sizeof(line), "%s%s",
                EIF_BASIC_AUTH_LINE_DEFAULT, b64_pass);
            if ((res < 0) || (res >= (int)EIF_BASIC_AUTH_LINE_MAX_LEN)) {
                ret = ESP_ERR_INVALID_SIZE;
            }
        }

        EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_nvs_value_save(
            NVS_KEY_BASIC_AUTH_LINE, line,
            EIF_BASIC_AUTH_LINE_MIN_LEN, EIF_BASIC_AUTH_LINE_MAX_LEN, false
        ), NVS_ERR_SAVE, "Basic Auth line");

        if (ret == ESP_OK) {
            EIF_LOG_I(NVS_MSG_SAVE_OK, "Basic Auth line");
        }

        /* Cleanup */
        if (b64_pass != NULL) {
            vPortFree(b64_pass);
        }
        return ret;
    }

    esp_err_t eif_nvs_basic_auth_line_load(char * const buf_out) {
        esp_err_t ret = ESP_OK;

        EIF_IF_OK_CHECK_NOT_NULL(ret, buf_out, ESP_ERR_INVALID_ARG);

        EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_nvs_value_load(
            NVS_KEY_BASIC_AUTH_LINE, buf_out, EIF_BASIC_AUTH_LINE_MAX_LEN
        ), NVS_ERR_LOAD, "Basic Auth line");

        if (ret == ESP_OK) {
            EIF_LOG_I(NVS_MSG_LOAD_OK, "Basic Auth line");
        }

        /* Cleanup */
        return ret;
    }
#endif

/* --- */

esp_err_t eif_nvs_initialize(void) {
    esp_err_t ret = ESP_OK;
    char wifi_ssid[EIF_WIFI_SSID_MAX_LEN] = {0};
    char wifi_pass[EIF_WIFI_PASS_MAX_LEN] = {0};
    #ifdef CONFIG_EIF_ENABLE_TLS
        char *tls_buffer_out = NULL;
        size_t tls_buffer_len = 0U;
    #endif
    #ifdef CONFIG_EIF_ENABLE_BASIC_AUTH
        char basic_auth_buffer[EIF_BASIC_AUTH_LINE_MAX_LEN] = {0};
    #endif
    const eif_core_t * const cfg = eif_core_get();

    EIF_IF_OK_CHECK_ESP_ERR_T(ret, nvs_flash_init(),
        NVS_ERR_OPEN_WRITE, "Flash Init");
    if ((ret == ESP_ERR_NVS_NO_FREE_PAGES)
        || (ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    ) {
        ret = ESP_OK;
        EIF_LOG_W("'Flash Init' re-creation launches");
        ret = nvs_flash_erase();
        if (ret != ESP_OK) {
            EIF_LOG_E(NVS_ERR_OPEN_WRITE, "Flash Erase");
        }
        if (ret == ESP_OK) {
            ret = nvs_flash_init();
            if (ret != ESP_OK) {
                EIF_LOG_E(NVS_ERR_OPEN_WRITE, "Flash Re-init");
            }
        }
    }

    if (ret == ESP_OK) {
        EIF_LOG_I("NVS Flash system is ready");

        /* @note The Wi-Fi profile with index 0 is hard-coded in the code
         * (read-only) and should not be overwritten or deleted.Thus, the
         * valid user-accessible range starts from 1. */
        for (size_t i = 1U; i <= (size_t)cfg->wifi_profiles_count; i++) {
            ret = eif_nvs_wifi_profile_load(i, wifi_ssid, wifi_pass);

            if (ret == ESP_ERR_NVS_NOT_FOUND) {
                EIF_LOG_W(NVS_ERR_MISSING_IDX, "Wi-Fi profile", i);
                ret = ESP_OK;

                EIF_IF_OK_CHECK_ESP_ERR_T(ret,
                    eif_nvs_wifi_profile_save(i, "", ""),
                    NVS_ERR_SAVE_F_IDX, "", i);
            } else if (ret != ESP_OK) {
                EIF_LOG_E("Loading Wi-Fi profile field '%d' failed, err: %s",
                    i, esp_err_to_name(ret));
            } else { ; }
            if (ret != ESP_OK) {
                break;
            }
        }
    }

    #ifdef CONFIG_EIF_ENABLE_BASIC_AUTH
        if (ret == ESP_OK) {
            ret = eif_nvs_basic_auth_line_load(basic_auth_buffer);

            if (ret == ESP_ERR_NVS_NOT_FOUND) {
                EIF_LOG_W(NVS_ERR_MISSING, "Basic Auth credentials");
                ret = ESP_OK;

                EIF_IF_OK_CHECK_ESP_ERR_T(ret,
                    eif_nvs_basic_auth_line_save((const unsigned char *)""),
                    NVS_ERR_SAVE, "Basic Auth credentials");
            } else if (ret != ESP_OK) {
                EIF_LOG_E("Loading 'Basic Auth credentials' failed, err: %s",
                    esp_err_to_name(ret));
            } else { ; }
        }
    #endif

    #ifdef CONFIG_EIF_ENABLE_TLS
        if (ret == ESP_OK) {
            ret = eif_nvs_value_load_malloc(
                EIF_NVS_KEY_TLS_CERT, &tls_buffer_out, &tls_buffer_len);
           
            if ((ret != ESP_OK) && (ret != ESP_ERR_NVS_NOT_FOUND)) {
                EIF_LOG_E("'TLS cert' loaded error: %s", esp_err_to_name(ret));
            }
            if (tls_buffer_out != NULL) {
                vPortFree(tls_buffer_out);
                tls_buffer_out = NULL;
            }

            ret = eif_nvs_value_load_malloc(
                EIF_NVS_KEY_TLS_PRIV_KEY, &tls_buffer_out, &tls_buffer_len);
            if ((ret != ESP_OK) && (ret != ESP_ERR_NVS_NOT_FOUND)) {
                EIF_LOG_E("'TLS key' loaded error: %s", esp_err_to_name(ret));
            }
            if (tls_buffer_out != NULL) {
                vPortFree(tls_buffer_out);
                tls_buffer_out = NULL;
            }

            if (ret == ESP_ERR_NVS_NOT_FOUND) {
                EIF_LOG_W(NVS_ERR_MISSING, "TLS credentials");

                /* @note Running a task instead of calling a function directly
                 * reduces the amount of stack required in the main thread. This
                 * decision is justified because the absence of TLS credentials
                 * is possible only when the device is first started. */
                EIF_SHOW_ESP_ERR_T(ret, eif_task_tls_recreate_launch(),
                    "Failed to spawn [tls_recreate]");

                if (ret == ESP_OK) {
                    /* @note Since the 'eif_task_reboot_launch' task forces a
                     * reboot of the device at the end of its operation, this
                     * cycle is safe for the system and does not cause a hang.
                     * But if the task is started unsuccessfully, this cycle
                     * will cause the system to freeze, meaning that it starts
                     * only on successful startup 'eif_task_reboot_launch' */
                    for (;;) {
                        vTaskDelay(portMAX_DELAY); 
                    }
                }
            } else if (ret != ESP_OK) {
                EIF_LOG_E("Loading 'TLS credentials' failed");
            } else { ; }
        }
    #endif

    if (ret == ESP_OK) {
        EIF_LOG_I("All NVS fields initialized successfully");
    }

    /* Cleanup */
    return ret;
}