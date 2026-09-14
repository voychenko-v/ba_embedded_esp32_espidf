#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "esp_log.h"

#define SERVO_PIN 18
#define SERVO_CHANNEL LEDC_CHANNEL_0
#define SERVO_TIMER LEDC_TIMER_0
#define SERVO_FREQ 50
#define SERVO_RESOLUTION LEDC_TIMER_12_BIT

#define POT_ADC_CHANNEL ADC_CHANNEL_6
#define ADC_ATTEN ADC_ATTEN_DB_12
#define ADC_BITWIDTH ADC_BITWIDTH_12
#define ADC_CAPACITY ((1 << ADC_BITWIDTH) - 1)

// Відкалібровані значення
#define MIN_PULSE_US 500    
#define MAX_PULSE_US 2500
#define PERIOD_US 20000

static adc_oneshot_unit_handle_t adc1_handle;

static inline int clamp_f(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

void setup_servo_ledc(void) {
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = SERVO_RESOLUTION,
        .timer_num = SERVO_TIMER,
        .freq_hz = SERVO_FREQ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    ledc_channel_config_t channel_conf = {
        .gpio_num = SERVO_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = SERVO_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = SERVO_TIMER,
        .duty = 0,   
        .hpoint = 0,
        .flags = {.output_invert = 0}
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));
}

void pot_sensor_init(void){
    
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_2,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, POT_ADC_CHANNEL, &config));
}

uint16_t pot_sensor_read_raw(void) {
    int adc_raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, POT_ADC_CHANNEL, &adc_raw));
    return (uint16_t)adc_raw;
}

static inline long map_range(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void set_servo_angle_ledc(int angle) {
    angle = clamp_f(angle, 0, 180);

    float pulse_us = MAX_PULSE_US - (angle / 180.0f) * (MAX_PULSE_US - MIN_PULSE_US);
    uint32_t duty = (uint32_t)((pulse_us / PERIOD_US) * (1 << SERVO_RESOLUTION));

    ledc_set_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL);
    
}

void app_main(void) {
    setup_servo_ledc();
    pot_sensor_init();

    while (1) {
        int raw = pot_sensor_read_raw();
        int angle = map_range(raw, 0, ADC_CAPACITY, 0, 180);
        set_servo_angle_ledc(angle);
        ESP_LOGI("SERVO", "raw=%d angle=%d", raw, angle);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
}