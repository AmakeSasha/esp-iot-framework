/* SPDX-License-Identifier: CC0-1.0 */

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

typedef struct {
    /* What is the stepper doing right now: STOP, UP, DOWN */
    stepper_dir_t current_dir;
    /* Power state on the driver (SLEEP_PIN) */
    bool is_powered;
    /* The number of steps taken since the start */
    uint32_t step_counter;
    /* The number of steps left to complete the move (0 for infinite) */
    uint32_t steps_to_move;
} stepper_state_t;

char* stepper_dir_to_str(stepper_dir_t dir);

void stepper_set_dir(stepper_dir_t dir);
void stepper_set_power(bool power);
void stepper_set_steps_to_move(uint32_t steps);

const volatile stepper_state_t* stepper_get_cfg(void);

void stepper_init(void);

#ifdef __cplusplus
    }
#endif

#endif
