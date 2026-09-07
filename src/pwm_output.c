#include "pwm_output.h"
#include "esp_err.h"

void pwm_output_init(const pwm_output_t *out, ledc_timer_bit_t duty_res) {
    ledc_timer_config_t timer_config = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = duty_res,
        .timer_num       = out->timer,
        .freq_hz         = out->freq_hz,
        .clk_cfg         = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

    ledc_channel_config_t channel_config = {
        .gpio_num   = out->gpio,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = out->channel,
        .timer_sel  = out->timer,
        .duty       = 0,
        .hpoint     = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_config));
}

void pwm_output_set_duty(const pwm_output_t *out, uint32_t duty) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, out->channel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, out->channel);
}