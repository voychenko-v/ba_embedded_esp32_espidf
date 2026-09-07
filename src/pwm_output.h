#pragma once
#include "driver/ledc.h"
#include "driver/gpio.h"

typedef struct {
    ledc_timer_t   timer;
    ledc_channel_t channel;
    gpio_num_t     gpio;
    uint32_t       freq_hz;
} pwm_output_t;

void pwm_output_init(const pwm_output_t *out, ledc_timer_bit_t duty_res);
void pwm_output_set_duty(const pwm_output_t *out, uint32_t duty);