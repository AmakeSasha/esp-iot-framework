/* SPDX-License-Identifier: CC0-1.0 */

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
