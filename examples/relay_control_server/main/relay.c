/*
 * SPDX-License-Identifier: Apache-2.0
 * Example: relay_control_server
 * Folder: ./examples/relay_control_server/main
 * File: relay.c
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

#include "relay.h"
#include <driver/gpio.h>

#define RELAY_PIN   13

static volatile relay_state_t g_relay_state = {
    .is_inversed = false,
    .is_closed = false,
    .number_of_changes = 0
};

/* Setters */
void relay_set_state(pin_state_t state) {
    gpio_set_level(RELAY_PIN, g_relay_state.is_inversed ? (int)state : (int)!state);
    
    if (g_relay_state.is_closed != state) {
        g_relay_state.number_of_changes++;
        g_relay_state.is_closed = state;
    }
}

void relay_toggle(void) {
    relay_set_state(!g_relay_state.is_closed);
}

void relay_set_inversed_logic(void) {
    g_relay_state.is_inversed = !g_relay_state.is_inversed;
}

/* Getters */
const volatile relay_state_t* relay_get_cfg(void) {
    return &g_relay_state;
}

/* --- */

void relay_init(bool is_inversed) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RELAY_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&io_conf);
    g_relay_state.is_inversed = is_inversed;
    gpio_set_level(RELAY_PIN, PIN_OFF); 
}
