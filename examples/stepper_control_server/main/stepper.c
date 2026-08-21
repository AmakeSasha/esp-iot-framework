/*
 * SPDX-License-Identifier: Apache-2.0
 * Example: stepper_control_server
 * Folder: ./examples/stepper_control_server/main
 * File: stepper.c
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

#include "stepper.h"
#include <soc/soc.h>
#include <driver/gpio.h>
#include <soc/gpio_reg.h>
#include <esp_idf_version.h>
#include <soc/gpio_struct.h>
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    #include <driver/gptimer.h>
#else
    #include <driver/timer.h>
#endif

#define STEP_PIN    13
#define SLEEP_PIN   14
#define DIR_PIN     27

#define STEP_MASK   (1ULL << STEP_PIN)
#define SLEEP_MASK  (1ULL << SLEEP_PIN)
#define DIR_MASK    (1ULL << DIR_PIN)

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    static gptimer_handle_t s_stepper_timer = NULL;
#else
    #define TIMER_GROUP_0 0
    #define TIMER_0       0
#endif

static volatile stepper_state_t g_stepper_state = {
    .current_dir = STEPPER_STOP,
    .is_powered = false,
    .step_counter = 0,
    .steps_to_move = 0
};

char* stepper_dir_to_str(stepper_dir_t dir) {
    switch (dir) {
        case STEPPER_STOP: return "STOP";
        case STEPPER_UP:   return "UP";
        case STEPPER_DOWN: return "DOWN";
    }
    return "UNKNOWN";
}

/* Setters */
void stepper_set_dir(stepper_dir_t dir) {
    g_stepper_state.current_dir = dir;
}

void stepper_set_power(bool power) {
    g_stepper_state.is_powered = power;
}

void stepper_set_steps_to_move(uint32_t steps) {
    g_stepper_state.steps_to_move = steps;
}

/* Getters */
const volatile stepper_state_t* stepper_get_cfg(void) {
    return &g_stepper_state;
}

/* --- */

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    static IRAM_ATTR bool stepper_timer_isr(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx) {
#else
    static IRAM_ATTR void  stepper_timer_isr(void *user_ctx) {
        timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);
        timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_0);
#endif
   static bool toggle = false;

    if (g_stepper_state.is_powered) {
        REG_WRITE(GPIO_OUT_W1TS_REG, SLEEP_MASK);
    } else {
        REG_WRITE(GPIO_OUT_W1TC_REG, SLEEP_MASK);
        
        if (toggle) {
            REG_WRITE(GPIO_OUT_W1TC_REG, STEP_MASK);
            toggle = false;
        }
        #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
            return false;
        #else
            return;
        #endif
    }

    stepper_dir_t dir = g_stepper_state.current_dir; 

    if (dir == STEPPER_STOP) {
        if (toggle) {
            REG_WRITE(GPIO_OUT_W1TC_REG, STEP_MASK);
            toggle = false;
        }
        #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
            return false;
        #else
            return;
        #endif
    }

    if (dir == STEPPER_UP) {
        REG_WRITE(GPIO_OUT_W1TS_REG, DIR_MASK);
    } else {
        REG_WRITE(GPIO_OUT_W1TC_REG, DIR_MASK);
    }

    if (!toggle) {
        REG_WRITE(GPIO_OUT_W1TS_REG, STEP_MASK);
        toggle = true;
    } else {
        REG_WRITE(GPIO_OUT_W1TC_REG, STEP_MASK);
        toggle = false;

        g_stepper_state.step_counter++;

        if (g_stepper_state.steps_to_move > 0) {
            g_stepper_state.steps_to_move--;
            if (g_stepper_state.steps_to_move == 0) {
                g_stepper_state.current_dir = STEPPER_STOP;
            }
        }
    }

    #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        return false;
    #else
        return;
    #endif
}

void stepper_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = STEP_MASK | DIR_MASK | SLEEP_MASK,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    g_stepper_state.current_dir = STEPPER_STOP;
    g_stepper_state.is_powered = false;
    REG_WRITE(GPIO_OUT_W1TC_REG, STEP_MASK | DIR_MASK | SLEEP_MASK);

    #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        gptimer_config_t timer_config = {
            .clk_src = GPTIMER_CLK_SRC_DEFAULT,
            .direction = GPTIMER_COUNT_UP,
            .resolution_hz = 1000000,
        };
        ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &s_stepper_timer));

        gptimer_event_callbacks_t cbs = { .on_alarm = stepper_timer_isr };
        ESP_ERROR_CHECK(gptimer_register_event_callbacks(s_stepper_timer, &cbs, NULL));
        ESP_ERROR_CHECK(gptimer_enable(s_stepper_timer));

        gptimer_alarm_config_t alarm_config = { .alarm_count = 150, .flags.auto_reload = true };
        ESP_ERROR_CHECK(gptimer_set_alarm_action(s_stepper_timer, &alarm_config));
        
        ESP_ERROR_CHECK(gptimer_start(s_stepper_timer));
    #else
        timer_config_t config = {
            .divider = 80,
            .counter_dir = TIMER_COUNT_UP,
            .counter_en = TIMER_PAUSE,
            .alarm_en = TIMER_ALARM_EN,
            .auto_reload = TIMER_AUTORELOAD_EN
        };
        ESP_ERROR_CHECK(timer_init(TIMER_GROUP_0, TIMER_0, &config));
        ESP_ERROR_CHECK(timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0));
        
        ESP_ERROR_CHECK(timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, 150));
        ESP_ERROR_CHECK(timer_enable_intr(TIMER_GROUP_0, TIMER_0));
        
        ESP_ERROR_CHECK(timer_isr_register(TIMER_GROUP_0, TIMER_0, stepper_timer_isr, NULL, ESP_INTR_FLAG_IRAM, NULL));
        
        ESP_ERROR_CHECK(timer_start(TIMER_GROUP_0, TIMER_0));
    #endif
}
