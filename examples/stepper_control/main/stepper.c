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

#define STEP_MASK   (1ULL << STEP_PIN)
#define SLEEP_MASK  (1ULL << SLEEP_PIN)
#define DIR_MASK    (1ULL << DIR_PIN)

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    static gptimer_handle_t s_stepper_timer = NULL;
#else
    #define TIMER_GROUP_0 0
    #define TIMER_0       0
#endif

volatile motor_state_t g_motor_state = {
    .current_dir = STOP,
    .is_powered = false,
    .step_counter = 0
};

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    static IRAM_ATTR bool stepper_timer_isr(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx) {
#else
    static IRAM_ATTR void  stepper_timer_isr(void *user_ctx) {
        timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);
        timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_0);
#endif
   static bool toggle = false;

    if (g_motor_state.is_powered) {
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

    motor_dir_t dir = g_motor_state.current_dir; 

    if (dir == STOP) {
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

    if (dir == UP) {
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

        g_motor_state.step_counter++; 
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

    g_motor_state.current_dir = STOP;
    g_motor_state.is_powered = false;
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
