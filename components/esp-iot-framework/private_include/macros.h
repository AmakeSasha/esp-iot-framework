/*
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp_iot_framework
 * Folder: src
 * File: macros.h
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

#ifndef MACROS_H
#define MACROS_H

#include "sdkconfig.h"

/* Other */

#define DEFINE_MIDDLEWARE(status, checker) \
static esp_err_t middleware_##status(httpd_req_t *req) { \
    uint32_t start_time = esp_log_timestamp(); \
    CORE_LOG(I, "%s %s HTTP/1.1", method_to_str(req->method), req->uri); \
    const httpd_uri_t* orig = (const httpd_uri_t*)req->user_ctx; \
    esp_err_t (*real_handler)(httpd_req_t *) = (esp_err_t (*)(httpd_req_t *))orig->handler; \
    esp_err_t err = checker(req); \
    if (err != ESP_OK) { \
        CORE_LOG(E, "Checker Basic Auth error: %s", esp_err_to_name(err)); \
        return ESP_OK; \
    } \
    if (real_handler) { \
        req->user_ctx = orig->user_ctx; \
        err = real_handler(req); \
        CORE_LOG(I, "Result(%u): %s", (size_t)start_time, esp_err_to_name(err)); \
        return err; \
    } \
    return ESP_FAIL; \
}

#ifdef CONFIG_EIF_ENABLE_WEB_ADMIN_GUI
    #define DEFINE_HTTP_FILE(name, filename_asm, resp_type, need_cache) \
        extern const uint8_t name##_start[] asm("_binary_" #filename_asm "_start"); \
        extern const uint8_t name##_end[]   asm("_binary_" #filename_asm "_end");   \
        static esp_err_t h_file_##name(httpd_req_t *req) { \
            const size_t size = (name##_end - name##_start); \
            CHECK_ESP_ERR_T(E, set_cache(req, need_cache), \
                if (err == ESP_WARN_CACHE_HIT) return ESP_OK, return err, \
                "Failed to set cache for %s", #name); \
            httpd_resp_set_type(req, resp_type); \
            httpd_resp_set_hdr(req, "Content-Encoding", "gzip"); \
            return httpd_resp_send(req, (const char *)name##_start, size); \
        }
#endif

#define GOTO_CLEANUP_ERR() do { \
    ret = err; \
    goto cleanup; \
} while(0)
    
/* Checkers */

#define __REL_FILE__ (strstr(__FILE__, "src") ? strstr(__FILE__, "src") : __FILE__)

#define LOG_LEVEL_E 1
#define LOG_LEVEL_W 2
#define LOG_LEVEL_I 3
#define LOG_LEVEL_D 4

#if CONFIG_EIF_LOG_SHOW_METADATA
    #define _FMT(msg) "%s:%d (%s) " msg, __REL_FILE__, __LINE__, __func__
#else
    #define _FMT(msg) msg
#endif

#define CORE_LOG(lvl, fmt, ...) do { \
    if (LOG_LEVEL_##lvl <= CONFIG_EIF_LOG_LEVEL) \
        ESP_LOG##lvl(TAG, _FMT(fmt), ##__VA_ARGS__); \
} while(0)

#ifdef CONFIG_EIF_ENABLE_TLS
    #define CHECK_MBEDTLS_ERR(ret, func_name) do { \
        if (ret != 0) { \
            char err_buf[100]; \
            mbedtls_strerror(ret, err_buf, sizeof(err_buf)); \
            CORE_LOG(E, _ERR_MBEDTLS, func_name, err_buf, -ret); \
            goto cleanup; \
        } \
    } while(0)
#endif
    
#define CHECK_CONDITION_WEB(condition, status, err, data_info, ...) do { \
	if (condition) { \
    	CORE_LOG(E, data_info, ##__VA_ARGS__); \
	    http_status = status; \
	    ret = err; \
	    goto cleanup; \
	} \
} while(0)

#define CHECK_NOT_NULL(ptr, err, error_block) do { \
    if ((ptr) == NULL) { \
    	CORE_LOG(E, "PARAMETER_IS_NULL: '%s'", #ptr); \
        ret = err; \
        { error_block; } \
    } \
} while(0)

#define CHECK_ESP_ERR_T(level, expr, every_block, error_block, data_info, ...) do { \
    esp_err_t err = (expr); \
    { every_block; } \
    if (err != ESP_OK) { \
        const char* err_name = esp_err_to_name(err); \
        CORE_LOG(level, "%s " data_info, \
            (err_name ? err_name : "UNKNOWN_ERROR"), ##__VA_ARGS__); \
        { error_block; } \
    } \
} while (0)

#endif