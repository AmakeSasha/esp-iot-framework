/*
 * SPDX-License-Identifier: Apache-2.0
 * Example: relay_control_server
 * Folder: ./examples/relay_control_server/main
 * File: relay.h
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

#ifndef STEPPER_H
#define STEPPER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct {
    /* Whether the hardware relay operates on inverted logic (LOW to close) or not */
    bool is_inversed;
    /* Whether current is flowing through the `RELAY_PIN` or not */
    bool is_closed; 
    /* Total count of all switching operations performed since system boot */
    uint32_t number_of_changes;
} relay_state_t;

typedef enum {
    PIN_ON  = 0,
    PIN_OFF = 1,
} pin_state_t;

void relay_set_state(pin_state_t state);
void relay_toggle(void);
void relay_set_inversed_logic(void);

/* Getters */
const volatile relay_state_t* relay_get_cfg(void);

void relay_init(bool is_inversed);

#ifdef __cplusplus
    }
#endif

#endif