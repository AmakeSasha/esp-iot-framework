/*
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Folder: ./components/esp_iot_framework_core/src
 * File: system.c
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

#include <string.h>
#include <esp_log.h>
#include <inttypes.h>
#include <driver/gpio.h>
#include <esp_ota_ops.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#ifdef CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG
    #include <freertos/ringbuf.h>
#endif

#include "private_include/macros.h"
#include "private_include/internal.h"

#define TASK_REBOOT_SIZE     (CONFIG_EIF_REBOOT_TASK_STACK_SIZE)
#define TASK_REBOOT_PRIORITY (configMAX_PRIORITIES - 1)

#define TASK_TLS_CREATING_SIZE     10240U
#define TASK_TLS_CREATING_PRIORITY 5

#ifdef CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG
    #define TASK_MEMORY_MONITOR_SIZE 1512
#else
    #define TASK_MEMORY_MONITOR_SIZE 1108
#endif
#define TASK_MEMORY_MONITOR_PRIORITY 5

/* --- */
#if CONFIG_EIF_MEM_MONITOR_CRITICAL_SIZE < (CONFIG_EIF_REBOOT_TASK_STACK_SIZE * 8)
    #error "EIF_MEM_MONITOR_CRITICAL_SIZE must be at least 8 times larger than EIF_REBOOT_TASK_STACK_SIZE!"
#endif

#define TAG "EIF System"

/* Logging */

#ifdef CONFIG_EIF_LOG_ENABLE_REMOTE_DEBUG
    static RingbufHandle_t core_log_buf = NULL;

    static int core_global_log_vprintf(const char *fmt, va_list l) {
        RingbufHandle_t buf = core_log_buf;

        if (buf != NULL) {
            char line_buf[192] = {0};
            va_list args = {0};

            va_copy(args, l);
            int len = vsnprintf(line_buf, sizeof(line_buf), fmt, args);
            va_end(args);
            
            if ((len > 0) && ((size_t)len < sizeof(line_buf))) {
                if (xRingbufferSend(buf, line_buf, len, 0) == pdFALSE) {
                    size_t attempts = 4U; 
                    bool is_sent = false;
                    bool has_data = true;

                    while ((attempts > 0U) && (!is_sent) && (has_data)) {
                        attempts--;

                        size_t dummy_size = 0;
                        char *dummy_ptr = (char *)xRingbufferReceiveUpTo(
                            buf, &dummy_size, 0, (size_t)len
                        );

                        if (dummy_ptr != NULL) {
                            vRingbufferReturnItem(buf, (void *)dummy_ptr);
                            
                            if (xRingbufferSend(buf, line_buf, (size_t)len, 0) == pdTRUE) {
                                is_sent = true;
                            }
                        } else {
                            has_data = false;
                        }
                    }
                }
            }
        }

        return vprintf(fmt, l);
    }

    void core_log_init(void) {
        core_log_buf = xRingbufferCreate(
            CONFIG_EIF_LOG_REMOTE_BUFFER_SIZE, RINGBUF_TYPE_BYTEBUF
        );
        if (core_log_buf) {
            esp_log_set_vprintf(core_global_log_vprintf);
        }
    }

    size_t core_log_pop_chunk(char *dest_buf, size_t max_size) {
        size_t res = 0;
        size_t item_size = 0;

        if (core_log_buf && dest_buf && (max_size != 0U)) {
            char *raw_chunk = (char *)xRingbufferReceiveUpTo(
                core_log_buf, &item_size, 0, max_size
            );

            if (raw_chunk && (item_size > 0U)) {
                (void)memcpy(dest_buf, raw_chunk, item_size);

                vRingbufferReturnItem(core_log_buf, (void *)raw_chunk);

                res = item_size;
            }
        }

        return res;
    }
#endif



/* Tasks */

static esp_err_t task_common_spawn(
    TaskHandle_t * const p_handle, const TaskFunction_t f_worker,
    const char * const p_name, const uint32_t u32_stack,
    const UBaseType_t u_prio
) {
    esp_err_t ret = ESP_OK;

    EIF_CHECK_NOT_NULL(p_handle, ESP_ERR_INVALID_ARG);
    EIF_CHECK_NOT_NULL(f_worker, ESP_ERR_INVALID_ARG);
    EIF_CHECK_NOT_NULL(p_name,   ESP_ERR_INVALID_ARG);

    if (*p_handle != NULL) {
        EIF_LOG_W("[%s] already exists", p_name);
        return ESP_ERR_INVALID_STATE;
    }

    if (xTaskCreate(f_worker, p_name, u32_stack, NULL, u_prio, p_handle) != pdPASS) {
        EIF_LOG_E("Failed to create [%s], free heap: %" PRIu32 " bytes",
                  p_name, esp_get_free_heap_size());

        *p_handle = NULL;
        ret = ESP_ERR_NO_MEM;
    }

cleanup:
    return ret;
}



/* ===================== EXCLUSIVE FORCED TASKS ====================== */
static TaskHandle_t x_exclusive_forced_handle = NULL;

/* ------ task_reboot ------ */
static void task_reboot(void *arg) {
    const eif_config_t * const cfg = eif_config_get();

    vTaskDelay(pdMS_TO_TICKS(100));
    if (cfg->sys_handler_reboot != NULL) {
        cfg->sys_handler_reboot();
    }
    esp_restart();

    /* Cleanup */
    x_exclusive_forced_handle = NULL;
    vTaskDelete(NULL);
}

void sys_launch_task_reboot(void) {
    if (task_common_spawn(
        &x_exclusive_forced_handle, &task_reboot, "task_reboot", TASK_REBOOT_SIZE, TASK_REBOOT_PRIORITY
    ) != ESP_OK) {
        EIF_LOG_E("Create ['task_reboot'] failed, force restart...");
        /* @note If the reboot task cannot be spawned, it indicates a critical system state
         *       (likely heap exhaustion). In this case, an immediate hardware restart is the only
         *       reliable way to recover the device. */
        esp_restart();
    }
}
/* =================== EXCLUSIVE FORCED TASKS END ==================== */



/* ================== EXCLUSIVE TLS TASKS ================== */
#ifdef CONFIG_EIF_ENABLE_TLS
    static void task_tls_generation_worker(void *arg) {
        esp_err_t ret = ESP_OK;
        TaskHandle_t main_task_handle = (TaskHandle_t)arg;
        
        char *buf_cert = NULL;
        size_t buf_cert_len = 0U;
        char *buf_priv_key = NULL;
        size_t buf_priv_key_len = 0U;

        EIF_LOG_I("TLS credentials generation started");

        EIF_TRY(tls_create_creds(&buf_cert, &buf_cert_len, &buf_priv_key, &buf_priv_key_len));
        EIF_TRY(nvs_tls_creds_save(buf_cert, buf_priv_key));
        
        EIF_LOG_I("TLS credentials generated");

    cleanup:
        if (ret != ESP_OK) {
            EIF_LOG_E("Failed to generate TLS credentials, err: %s", esp_err_to_name(ret));
        }
        if (buf_cert != NULL) {
            vPortFree(buf_cert);
        }
        if (buf_priv_key != NULL) {
            vPortFree(buf_priv_key);
        }
        if (main_task_handle != NULL) {
            xTaskNotifyGive(main_task_handle);
        }
        vTaskDelete(NULL); 
    }

    esp_err_t sys_generate_tls_credentials_sync(void) {
        esp_err_t ret = ESP_OK;

        if (xTaskCreate(
            task_tls_generation_worker, "tls_gen", TASK_TLS_CREATING_SIZE,
            (void *)xTaskGetCurrentTaskHandle(), TASK_TLS_CREATING_PRIORITY, NULL
        ) != pdPASS) {
            EIF_LOG_E("Failed to create TLS generation task");
            return ESP_ERR_NO_MEM;
        }

        (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        return ret;
    }
#endif /* CONFIG_EIF_ENABLE_TLS */
/* ================ EXCLUSIVE TLS TASKS END ================ */



/* ================== EXCLUSIVE MEMORY MONITOR TASKS ================== */
#define MEM_MONITOR_INTERVAL_MS 500U

static TaskHandle_t x_service_task_handle = NULL;
/* ------ memory_monitor ------ */
static void task_memory_monitor(void *arg) {
    uint8_t count_critical_checks = 0;

    EIF_LOG_I("Memory monitor started. Critical: %u bytes, Max checks: %u",
        (int)CONFIG_EIF_MEM_MONITOR_CRITICAL_SIZE,
        (int)CONFIG_EIF_MEM_MONITOR_NUMBER_CHECKS);

    for (;;) {
        size_t largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);

        if (largest_block < (size_t)CONFIG_EIF_MEM_MONITOR_CRITICAL_SIZE) {
            count_critical_checks++;

            EIF_LOG_W("Low memory condition. Largest block %u < %u bytes, check %u/%u",
                (unsigned int)largest_block, CONFIG_EIF_MEM_MONITOR_CRITICAL_SIZE,
                (unsigned int)count_critical_checks, CONFIG_EIF_MEM_MONITOR_NUMBER_CHECKS);

            if (count_critical_checks >= (uint8_t)CONFIG_EIF_MEM_MONITOR_NUMBER_CHECKS) {
                EIF_LOG_E("Critical memory limit reached. Rebooting system...");

                x_exclusive_forced_handle = NULL;
                task_reboot(NULL);
                x_service_task_handle = NULL;

                vTaskDelete(NULL);
            }
        } else {
            count_critical_checks = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(MEM_MONITOR_INTERVAL_MS));
    }
}

esp_err_t sys_launch_task_memory_monitor(void) {
    return task_common_spawn(
        &x_service_task_handle, &task_memory_monitor, "task_memory_monitor",
        TASK_MEMORY_MONITOR_SIZE, TASK_MEMORY_MONITOR_PRIORITY);
}
/* ================ EXCLUSIVE MEMORY MONITOR TASKS END ================ */