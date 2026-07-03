/* SPDX-License-Identifier: Apache-2.0
 * Project: esp-iot-framework
 * Example: stepper_control
 * Folder: ./examples/stepper_control/main
 * File: stepper.h
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

typedef enum {
    STEPPER_STOP = 0,
    STEPPER_UP,
    STEPPER_DOWN
} stepper_dir_t;

char* stepper_dir_to_str(stepper_dir_t dir);

void stepper_set_dir(stepper_dir_t dir);
void stepper_set_power(bool power);
void stepper_set_steps_to_move(uint32_t steps);

stepper_dir_t stepper_get_dir(void);
bool stepper_get_power(void);
uint32_t stepper_get_step_counter(void);
uint32_t stepper_get_steps_to_move(void);

void stepper_init(void);

#ifdef __cplusplus
    }
#endif

#endif
