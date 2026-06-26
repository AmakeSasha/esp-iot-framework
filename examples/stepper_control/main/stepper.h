#ifndef STEPPER_H
#define STEPPER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
    extern "C" {
#endif

#define STEP_PIN    13
#define SLEEP_PIN   14
#define DIR_PIN     27

typedef enum {
    STOP = 0,
    UP,
    DOWN
} motor_dir_t;

typedef struct {
    /* What is the motor doing right now: STOP, UP, DOWN */
    motor_dir_t current_dir;
    /* Power state on the driver (SLEEP_PIN) */
    bool is_powered;
    /* The number of steps taken since the start */
    uint32_t step_counter;
} motor_state_t;

extern volatile motor_state_t g_motor_state;

void stepper_init(void);

#ifdef __cplusplus
    }
#endif

#endif // STEPPER_H
