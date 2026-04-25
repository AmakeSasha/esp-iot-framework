/*
 * SPDX-License-Identifier: Apache-2.0
 * Project: esp_iot_framework
 * Folder: src
 * File: system.c
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

#include "esp_log.h"
#include "esp_ota_ops.h"

#include "macros.h"
#include "core_internal.h"

#if CONFIG_EIF_MEM_MONITOR_CRITICAL_SIZE < (CONFIG_EIF_REBOOT_TASK_STACK_SIZE * 8)
    #error "EIF_MEM_MONITOR_CRITICAL_SIZE must be twice as large as EIF_REBOOT_TASK_STACK_SIZE!"
#endif

#define TAG "reboot_task"
void system_reboot_prepare(void) {
    esp_err_t ret = ESP_OK;
    const eif_t *cfg = eif_get();

    CORE_LOG(I, "Preparing for system reboot...");

    eif_wifi_handler_stop_set(true);
    vTaskDelay(pdMS_TO_TICKS(10));
    eif_server_stop();
    vTaskDelay(pdMS_TO_TICKS(100));

    ret = esp_wifi_disconnect();
    if (ret != ESP_OK) CORE_LOG(W, "esp_wifi_disconnect: %s", esp_err_to_name(ret));

    vTaskDelay(pdMS_TO_TICKS(100));
    ret = esp_wifi_stop();
    if (ret != ESP_OK) CORE_LOG(W, "esp_wifi_stop: %s", esp_err_to_name(ret));

    vTaskDelay(pdMS_TO_TICKS(100));
    ret = esp_wifi_deinit();
    if (ret != ESP_OK) CORE_LOG(W, "esp_wifi_deinit: %s", esp_err_to_name(ret));

    vTaskDelay(pdMS_TO_TICKS(100));

    if (cfg->user_pre_reboot_cb != NULL) {
        CORE_LOG(I, "Running 'user_pre_reboot_cb'");
        cfg->user_pre_reboot_cb();
    } else {
        CORE_LOG(W, "No set 'user_pre_reboot_cb', skipping");
    }

    CORE_LOG(I, "Reboot preparation complete.");
}

void reboot_task(void *pvParameters) {
    CORE_LOG(D, "Run task 'reboot_task'");
    vTaskDelay(pdMS_TO_TICKS(100));
    system_reboot_prepare();
    vTaskDelay(pdMS_TO_TICKS(300));
    CORE_LOG(W, "Reboot system...");
    esp_restart();
}
#undef TAG

#define TAG "rollback_and_reboot_task"
void rollback_and_reboot_task(void *pvParameters) {
    CORE_LOG(D, "Run task 'rollback_and_reboot_task'");
    vTaskDelay(pdMS_TO_TICKS(100));
    system_reboot_prepare();
    vTaskDelay(pdMS_TO_TICKS(300));
    CORE_LOG(W, "Reboot system...");
    esp_ota_mark_app_invalid_rollback_and_reboot();
}
#undef TAG

#define TAG "Memory monitor"
void memory_monitor_task(void *pvParameters) {
    uint8_t count_critical_checks = 0;

    CORE_LOG(I, "Memory monitor task started");
    CORE_LOG(I, "Interval: %d ms, Critical size: %d bytes, Checks needed: %d",
        CONFIG_EIF_MEM_MONITOR_CHECK_INTERVAL,
        CONFIG_EIF_MEM_MONITOR_CRITICAL_SIZE,
        CONFIG_EIF_MEM_MONITOR_NUMBER_CHECKS);

    while (1) {
        size_t largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
        bool is_critical = largest_block < CONFIG_EIF_MEM_MONITOR_CRITICAL_SIZE;

        #ifdef CONFIG_EIF_LOG_ENABLE_MEM_MONITOR
            size_t heap_free = esp_get_free_heap_size();
            CORE_LOG(I, "free=%u, largest=%u, critical=%s",
                heap_free, largest_block, is_critical ? "YES" : "NO");
        #endif

        if (is_critical) {
            count_critical_checks++;

            CORE_LOG(W, 
                "Memory pressure detected. Largest block %zu < %d bytes, %d/%d checks",
                largest_block, CONFIG_EIF_MEM_MONITOR_CRITICAL_SIZE,
                count_critical_checks, CONFIG_EIF_MEM_MONITOR_NUMBER_CHECKS);

            if (count_critical_checks >= CONFIG_EIF_MEM_MONITOR_NUMBER_CHECKS) {
                CORE_LOG(E, "CRITICAL MEMORY! Initiating reboot the system...");

                int result = xTaskCreate(reboot_task, "reboot_task",
                    CONFIG_EIF_REBOOT_TASK_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
                if (result != pdPASS) esp_restart();

                vTaskDelete(NULL); 
            }
        } else {
            count_critical_checks = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(CONFIG_EIF_MEM_MONITOR_CHECK_INTERVAL));
    }
}
#undef TAG