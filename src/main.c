#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include <driver/ledc.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
#include <esp_log.h>

#include "sma_filter.h"

#define ADC_CHANNEL     ADC_CHANNEL_6   // Pin 17
#define ADC_UNIT        ADC_UNIT_2
#define ADC_ATTEN       ADC_ATTEN_DB_12
#define ADC_BITWIDTH    ADC_BITWIDTH_12

#define SERVO_PIN           18
#define SERVO_FREQ          50
#define SERVO_PERIOD_MS     (1000/SERVO_FREQ)
#define SERVO_RESOLUTION    LEDC_TIMER_12_BIT
#define SERVO_MAX_DUTY      ((1 << SERVO_RESOLUTION) - 1)
#define SERVO_UNIT          LEDC_TIMER_0
#define SERVO_CHANNEL       LEDC_CHANNEL_0

#define SUPERLOOP_DELAY     100
#define WINDOW_SIZE  5

// Calibration variables
static int SERVO_MIN_DEG = 10;
static int SERVO_MAX_DEG = 170;

static int SERVO_MIN_US = 500;
static int SERVO_MAX_US = 2500;

static int LIGHT_MIN_MV = 50;
static int LIGHT_MAX_MV = 3000;

adc_oneshot_unit_handle_t adc_handle;
adc_cali_handle_t cali_handle;


static const char *TAG = "Mini Project";


static void ldr_init(void)
{
    adc_oneshot_unit_init_cfg_t unit_config = {
        .unit_id = ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_config, &adc_handle));

    adc_oneshot_chan_cfg_t channel_config = {
    .atten = ADC_ATTEN,
    .bitwidth = ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(
        adc_handle,
        ADC_CHANNEL,
        &channel_config
    ));


    adc_cali_curve_fitting_config_t cfg = {
        .unit_id = ADC_UNIT,
        .chan = ADC_CHANNEL,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cfg, &cali_handle));
}

static void servo_init(void)
{
    ledc_timer_config_t timer_config = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = SERVO_UNIT,
        .duty_resolution  = SERVO_RESOLUTION,
        .freq_hz          = SERVO_FREQ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

    ledc_channel_config_t channel_config = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = SERVO_CHANNEL,
        .timer_sel      = SERVO_UNIT,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = SERVO_PIN,
        .duty           = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_config));
}

static int val_clamp(int value, int val_min, int val_max)
{
    if (value < val_min) return val_min;
    if (value > val_max) return val_max;
    return value;
}

static int mad_map(int value, int in_min, int in_max, int out_min, int out_max)
{
    if (in_min == in_max) return out_max;
    value = val_clamp(value, in_min, in_max);
    return out_min + (value - in_min)*(out_max - out_min) / (in_max - in_min);
}

static void set_angle(int mv)
{
    mv = val_clamp(mv, LIGHT_MIN_MV, LIGHT_MAX_MV);

    int val_deg = mad_map(mv, LIGHT_MIN_MV, LIGHT_MAX_MV, SERVO_MIN_DEG, SERVO_MAX_DEG);
    ESP_LOGI(TAG, "val_deg = %d\n", val_deg);
    int val_us = mad_map(val_deg, SERVO_MIN_DEG, SERVO_MAX_DEG, SERVO_MIN_US, SERVO_MAX_US);
    ESP_LOGI(TAG, "val_us = %d\n", val_us);
    ESP_LOGI(TAG, "SERVO_MAX_DUTY = %d\n", SERVO_MAX_DUTY);

    uint32_t duty = (uint32_t)(SERVO_MAX_DUTY / SERVO_PERIOD_MS) * val_us / 1000; 
    ESP_LOGI(TAG, "Duty = %ld\n", duty);

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL));
}


void app_main(void)
{
    int raw;
    int mv;
    
    ldr_init();
    servo_init();
    
    // SMA init
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw));
    static int buffer[WINDOW_SIZE];
    sma_filter_t filter;
    sma_filter_init(&filter, buffer, WINDOW_SIZE, raw);



    while(1)
    {
        // ADT Test
        adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw);
        int sma = sma_filter_update(&filter, raw);
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, sma, &mv));
        ESP_LOGI(TAG, "Raw ADC Value: %d\tVoltage Value: %d mV\n", raw, mv);

        set_angle(mv);
        vTaskDelay(pdMS_TO_TICKS(SUPERLOOP_DELAY));

    }

}