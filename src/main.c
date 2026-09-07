#include "pwm_output.h"
#include "pot_sensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_FREQUENCY   1000     // 1 kHz
#define MOTOR_FREQUENCY 20000    // 20 kHz
#define DUTY_RESOLUTION LEDC_TIMER_10_BIT
#define DUTY_CAPACITY   ((1 << DUTY_RESOLUTION) - 1)

#define LED_PIN   1
#define MOTOR_PIN 2

static const pwm_output_t led_out   = { LEDC_TIMER_0, LEDC_CHANNEL_0, LED_PIN, LED_FREQUENCY };
static const pwm_output_t motor_out = { LEDC_TIMER_1, LEDC_CHANNEL_1, MOTOR_PIN, MOTOR_FREQUENCY };

void app_main(void) {
    pwm_output_init(&led_out, DUTY_RESOLUTION);
    pwm_output_init(&motor_out, DUTY_RESOLUTION);
    pot_sensor_init();

    while (1) {
        uint16_t duty = pot_sensor_read_scaled(DUTY_CAPACITY);
        pwm_output_set_duty(&led_out, duty);
        pwm_output_set_duty(&motor_out, duty);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}