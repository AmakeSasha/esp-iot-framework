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
uint32_t stepper_get_steps_to_move(void);
uint32_t stepper_get_steps_counter(void);

void stepper_init(void);

#ifdef __cplusplus
    }
#endif

#endif
