/* SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Library: esp_iot_framework_core
 * Folder: components/esp_iot_framework_core/src
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
#include <inttypes.h>
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_iot_framework_core_macros.h"
#include "core_internal.h"

#define ERR_SPAWN_TASK "Failed to spawn task [%s]. Free heap: %" PRIu32 " bytes"
#define ERR_TWO_SPAWN  "Task [%s] has already been created earlier and is running"
#define MSG_SPAWN_TASK "Spawn task [%s]"
#define MSG_CALL_FUNC "Calling the function '%s'"

#define TASK_REBOOT_NAME "t_reboot"
#define TASK_REBOOT_SIZE CONFIG_EIF_REBOOT_TASK_STACK_SIZE
#define TASK_REBOOT_PRIORITY configMAX_PRIORITIES - 1

#define TASK_MEMORY_MONITOR_NAME "t_memory_monitor"
#define TASK_MEMORY_MONITOR_SIZE 2048
#define TASK_MEMORY_MONITOR_PRIORITY 5

#define TASK_TLS_RECREATE_NAME "t_tls_recreate"
#define TASK_TLS_RECREATE_SIZE 10240
#define TASK_TLS_RECREATE_PRIORITY 5

#define TASK_ROLLBACK_AND_REBOOT_NAME "t_rollback_reboot"
#define TASK_ROLLBACK_AND_REBOOT_SIZE 4096
#define TASK_ROLLBACK_AND_REBOOT_PRIORITY 5

#define TASK_WIFI_TEST_NAME "t_wifi_tesr"
#define TASK_WIFI_TEST_SIZE 4096
#define TASK_WIFI_TEST_PRIORITY 5

/* --- */
#if CONFIG_EIF_MEM_MONITOR_CRITICAL_SIZE < (CONFIG_EIF_REBOOT_TASK_STACK_SIZE * 8)
    #error "EIF_MEM_MONITOR_CRITICAL_SIZE must be twice as large as EIF_REBOOT_TASK_STACK_SIZE!"
#endif

esp_err_t eif_task_common_spawn(
    TaskHandle_t * const p_handle, const TaskFunction_t f_worker,
    const char * const p_name, const uint32_t u32_stack,
    const UBaseType_t u_prio
) {
    EIF_TAG_WITH_UNUSED "Task spawner";

    esp_err_t ret = ESP_OK;

    EIF_LOG_D(MSG_CALL_FUNC, __func__);

    EIF_IF_OK_CHECK_NOT_NULL(ret, p_handle, ESP_ERR_INVALID_ARG);
    EIF_IF_OK_CHECK_NOT_NULL(ret, f_worker, ESP_ERR_INVALID_ARG);
    EIF_IF_OK_CHECK_NOT_NULL(ret, p_name, ESP_ERR_INVALID_ARG);
    
    if (ret == ESP_OK) {
        if (*p_handle != NULL) {
            EIF_LOG_W(ERR_TWO_SPAWN, p_name);
            ret = ESP_ERR_INVALID_STATE;
        }
    }
    
    if (ret == ESP_OK) {
        const BaseType_t x_res = xTaskCreate(f_worker,
            p_name, u32_stack, NULL, u_prio, p_handle);

        if (x_res != pdPASS) {
            EIF_LOG_E(ERR_SPAWN_TASK, p_name, esp_get_free_heap_size());

            *p_handle = NULL;
            ret = ESP_ERR_NO_MEM;
        }
    }

    return ret;
}

/* ------ Sys Reboot Prepare ------ */
static void eif_system_reboot_prepare(void) {
    EIF_TAG_WITH_UNUSED "Sys Reboot Prepare";

    esp_err_t ret = ESP_OK;
    const eif_core_t * const cfg = eif_core_get();

    EIF_LOG_D(MSG_CALL_FUNC, __func__);

    EIF_SHOW_ESP_ERR_T(ret, eif_wifi_deinitialize(),
        "Wi-Fi module deinitialization failed");

    vTaskDelay(pdMS_TO_TICKS(100));
    if (cfg->handler_system_reboot != NULL) {
        EIF_LOG_I("Running 'handler_system_reboot'");
        EIF_SHOW_ESP_ERR_T(ret, cfg->handler_system_reboot(),
            "Failed to execute 'SYSTEM_EVENT_REBOOT' handler");
    } else {
        EIF_LOG_W("No set 'handler_system_reboot', skipping");
    }

    EIF_LOG_I("Reboot preparation complete.");
    (void)ret;
}



/* ===================== EXCLUSIVE FORCED TASKS ====================== */
static TaskHandle_t x_eif_exclusive_forced_handle = NULL;

/* ------ task_reboot ------ */
static void eif_task_reboot(void *arg) {
    EIF_TAG_WITH_UNUSED "task_reboot";
    (void)arg;

    EIF_LOG_I(MSG_SPAWN_TASK, __func__);
    vTaskDelay(pdMS_TO_TICKS(100));
    eif_system_reboot_prepare();
    vTaskDelay(pdMS_TO_TICKS(300));
    EIF_LOG_W("Reboot system...");
    esp_restart();

    /* Cleanup */
    x_eif_exclusive_forced_handle = NULL; 
    vTaskDelete(NULL);
}

esp_err_t eif_task_reboot_launch(void) {
    EIF_TAG_WITH_UNUSED "task_reboot";

    EIF_LOG_D(MSG_CALL_FUNC, __func__);

    esp_err_t ret = ESP_OK;
    EIF_TASK_LAUNCH(ret, x_eif_exclusive_forced_handle,
        REBOOT, &eif_task_reboot);
    if (ret != ESP_OK) {
        EIF_LOG_E("Reboot task failed (err: %s). Force restart...", 
            esp_err_to_name(ret));
        /* @note If the reboot task cannot be spawned, it indicates a critical
         * system state (likely heap exhaustion). In this case, an immediate 
         * hardware restart is the only reliable way to recover the device. */
        esp_restart();
    }

    /* Cleanup */
    return ret;
}
/* =================== EXCLUSIVE FORCED TASKS END ==================== */



/* ======================= EXCLUSIVE SYS TASKS ======================= */
static TaskHandle_t x_eif_exclusive_sys_handle = NULL;

/* ------ tls_recreate ------ */
static void eif_task_tls_recreate(void* arg) {
    EIF_TAG_WITH_UNUSED "tls_recreate";
    (void)arg;

    esp_err_t ret = ESP_OK;

    EIF_LOG_I(MSG_SPAWN_TASK, __func__);
    vTaskDelay(pdMS_TO_TICKS(500));
    
    EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_tls_create_creds_and_nvs_save(), "qwe"); 
    if (ret == ESP_OK) {
        EIF_LOG_I("TLS credentials recreated. System will restart...");
        vTaskDelay(pdMS_TO_TICKS(500));
        EIF_LOG_I("Restarting...");
        (void)eif_task_reboot_launch();
    }

    /* Cleanup */
    x_eif_exclusive_sys_handle = NULL; 
    vTaskDelete(NULL);
}

esp_err_t eif_task_tls_recreate_launch(void) {
    esp_err_t ret = ESP_OK;

    EIF_LOG_D(MSG_CALL_FUNC, __func__);

    EIF_TASK_LAUNCH(ret, x_eif_exclusive_sys_handle,
        TLS_RECREATE, &eif_task_tls_recreate);

    /* Cleanup */
    return ret;
}

/* ------ rollback_and_reboot ------ */
static void eif_task_rollback_and_reboot(void *arg) {
    EIF_TAG_WITH_UNUSED "rollback_and_reboot";
    (void)arg;

    EIF_LOG_I(MSG_SPAWN_TASK, __func__);
    vTaskDelay(pdMS_TO_TICKS(100));
    eif_system_reboot_prepare();
    vTaskDelay(pdMS_TO_TICKS(300));
    EIF_LOG_W("Reboot system...");
    esp_ota_mark_app_invalid_rollback_and_reboot();

    /* Cleanup */
    x_eif_exclusive_sys_handle = NULL; 
    vTaskDelete(NULL);
}

esp_err_t eif_task_rollback_and_reboot_launch(void) {
    esp_err_t ret = ESP_OK;

    EIF_LOG_D(MSG_CALL_FUNC, __func__);

    EIF_TASK_LAUNCH(ret, x_eif_exclusive_sys_handle,
        ROLLBACK_AND_REBOOT, &eif_task_rollback_and_reboot);

    /* Cleanup */
    return ret;
}

/* ------ wifi_test ------ */
static uint8_t wifi_test_target_idx = 0U;
static void eif_task_wifi_test(void *arg) {
    EIF_TAG_WITH_UNUSED "wifi_test";
    (void)arg;

    esp_err_t ret = ESP_OK;
    esp_netif_ip_info_t ip_info = {0};
    wifi_ap_record_t info = {0};

    const eif_core_t * const cfg = eif_core_get();
    uint8_t origin_index = cfg->current_wifi_profile_index;
    uint8_t new_index = wifi_test_target_idx;
    eif_wifi_test_result res = { 
        .connected = false,
        .rssi = -127
    };
    
    vTaskDelay(pdMS_TO_TICKS(500)); 
    EIF_LOG_I("Starting WiFi test for param #%zu", (size_t)new_index);
    eif_wifi_handler_stop_set(true);
    EIF_IF_OK_CHECK_ESP_ERR_T(ret, esp_wifi_disconnect(), 
        "Disconnecting from current AP failed");

    if (ret == ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_wifi_set_config_from_profile(new_index), 
        "Applying configuration for param #%zu failed", (size_t)new_index);
    EIF_IF_OK_CHECK_ESP_ERR_T(ret, esp_wifi_connect(),
        "Initiating connection to param #%zu failed", (size_t)new_index);

    for (int i = 0; (ret == ESP_OK) && (i < 30); i++) {
        vTaskDelay(pdMS_TO_TICKS(500));
         
        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        esp_err_t ip_err = esp_netif_get_ip_info(netif, &ip_info);

        if (ip_err != ESP_OK) {
            EIF_LOG_W("Failed to get IP info: %s", esp_err_to_name(ip_err));
        } else {
            if ((netif) && (ip_info.ip.addr != 0U)) {
                res.connected = true;

                if (esp_wifi_sta_get_ap_info(&info) == ESP_OK) {
                    res.rssi = info.rssi;
                    break;
                }
            }
        }

        EIF_LOG_D("Waiting for IP address... (%d/30)", i + 1);
    }

    /* Cleanup */
    EIF_IF_OK_CHECK_ESP_ERR_T(ret, eif_set_wifi_result_test(new_index, res),
        "Failed to save WiFi test result");

    EIF_LOG_I("Restoring connection to param #%zu", origin_index);

    EIF_SHOW_ESP_ERR_T(ret, esp_wifi_disconnect(),
        "Disconnect request failed");
    vTaskDelay(pdMS_TO_TICKS(100));
    EIF_SHOW_ESP_ERR_T(ret, eif_wifi_set_config_from_profile(origin_index),
        "Restoring original config failed");
    vTaskDelay(pdMS_TO_TICKS(100));
    EIF_SHOW_ESP_ERR_T(ret, esp_wifi_connect(),
        "Reconnecting to original AP failed");

    (void)eif_wifi_handler_stop_set(false);

    /* Cleanup */
    x_eif_exclusive_sys_handle = NULL; 
    vTaskDelete(NULL);
}

esp_err_t eif_task_wifi_test_launch(uint8_t profile_index) {
    esp_err_t ret = ESP_OK;

    EIF_LOG_D(MSG_CALL_FUNC, __func__);

    wifi_test_target_idx = profile_index;
    EIF_TASK_LAUNCH(ret, x_eif_exclusive_sys_handle,
        WIFI_TEST, &eif_task_wifi_test);

    /* Cleanup */
    return ret;
}
/* ===================== EXCLUSIVE SYS TASKS END ====================== */



/* ================== EXCLUSIVE MEMORY MONITOR TASKS ================== */
static TaskHandle_t x_eif_service_task_handle = NULL;
/* ------ memory_monitor ------ */
static void eif_task_memory_monitor(void *arg) {
    EIF_TAG_WITH_UNUSED "memory_monitor";
    (void)arg;

    uint8_t count_critical_checks = 0;
    bool critical_fragmentation = false;

    EIF_LOG_I(MSG_SPAWN_TASK, __func__);
    EIF_LOG_I("Interval: %d ms, Critical size: %d bytes, Checks needed: %d",
        CONFIG_EIF_MEM_MONITOR_CHECK_INTERVAL,
        CONFIG_EIF_MEM_MONITOR_CRITICAL_SIZE,
        CONFIG_EIF_MEM_MONITOR_NUMBER_CHECKS);

    while (!critical_fragmentation) {
        size_t largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
        bool is_critical = largest_block < CONFIG_EIF_MEM_MONITOR_CRITICAL_SIZE;

        #ifdef CONFIG_EIF_LOG_ENABLE_MEM_MONITOR
            size_t heap_free = esp_get_free_heap_size();
            EIF_LOG_I("free=%u, largest=%u, critical=%s",
                heap_free, largest_block, is_critical ? "YES" : "NO");
        #endif

        if (is_critical) {
            count_critical_checks++;

            EIF_LOG_W(
                "Memory pressure detected. Largest block %zu < %d bytes, %d/%d checks",
                largest_block, CONFIG_EIF_MEM_MONITOR_CRITICAL_SIZE,
                count_critical_checks, CONFIG_EIF_MEM_MONITOR_NUMBER_CHECKS);

            if (count_critical_checks 
                >= (uint8_t)CONFIG_EIF_MEM_MONITOR_NUMBER_CHECKS) {
                EIF_LOG_E("CRITICAL MEMORY! Initiating reboot the system...");

                critical_fragmentation = true;
            }
        } else {
            count_critical_checks = 0;
        }

        if (!critical_fragmentation) {
            vTaskDelay(pdMS_TO_TICKS(CONFIG_EIF_MEM_MONITOR_CHECK_INTERVAL));
        }
    }

    /* Cleanup */
    x_eif_exclusive_sys_handle = NULL;
    (void)eif_task_reboot_launch();
    x_eif_service_task_handle = NULL;
    vTaskDelete(NULL); 
}

esp_err_t eif_task_memory_monitor_launch(void) {
    esp_err_t ret = ESP_OK;

    EIF_LOG_D(MSG_CALL_FUNC, __func__);
    
    EIF_TASK_LAUNCH(ret, x_eif_service_task_handle,
        MEMORY_MONITOR, &eif_task_memory_monitor);

    /* Cleanup */
    return ret;
}
/* ================ EXCLUSIVE MEMORY MONITOR TASKS END ================ */