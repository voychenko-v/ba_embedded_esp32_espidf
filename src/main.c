#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "driver/ledc.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "esp_timer.h"

// =====================================================================

#define ENCODER_GPIO_A 9
#define ENCODER_GPIO_B 10
#define ENCODER_BUTTON_GPIO   4
#define PCNT_UNIT PCNT_UNIT_0

#define DEBOUNCE_TIME_US 50000

#define SERVO_PIN 18
#define SERVO_CHANNEL LEDC_CHANNEL_0
#define SERVO_TIMER LEDC_TIMER_0
#define SERVO_FREQ 50
#define SERVO_RESOLUTION LEDC_TIMER_12_BIT
#define START_ANGLE 90

#define MIN_PULSE_US 500
#define MAX_PULSE_US 2500
#define PERIOD_US 20000

#define BUZZER_PIN 5
#define BUZZER_CHANNEL LEDC_CHANNEL_1
#define BUZZER_TIMER LEDC_TIMER_1
#define BUZZER_FREQ_HZ 2500
#define BUZZER_RESOLUTION LEDC_TIMER_10_BIT
#define BEEP_DURATION_MS 100

// =====================================================================

static int switching_angle = 40;
static int current_angle = START_ANGLE;
static int current_encoder_value = 0;
static int previosly_encoder_value = 0;
static int accumulated_delta = 0;
static esp_timer_handle_t buzzer_timer;

static volatile bool half_step_mode = false;
static volatile int64_t last_isr_time = 0;

static pcnt_unit_handle_t pcnt_unit = NULL;
static pcnt_channel_handle_t pcnt_channel_0 = NULL;
static pcnt_channel_handle_t pcnt_channel_1 = NULL;

// =====================================================================

static void IRAM_ATTR button_isr_handler(void* arg);

static inline int clamp_f(int v, int lo, int hi);

void setup_encoder_pcnt(void);
void setup_encoder_button(void);
void setup_servo_ledc(void);
void setup_buzzer(void);

static void buzzer_off_cb(void* arg);
void trigger_beep(void);

void set_servo_angle_ledc(int angle);


// =====================================================================

void app_main(void) {
    setup_encoder_pcnt();
    setup_encoder_button();
    setup_servo_ledc();
    setup_buzzer();
    set_servo_angle_ledc(START_ANGLE);

    while (1) {
        ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &current_encoder_value));

        int delta = current_encoder_value - previosly_encoder_value;
        if (delta != 0) {
            previosly_encoder_value = current_encoder_value;
            accumulated_delta += delta;
            ESP_LOGI("ENCODER", "count: %d, delta: %d, accumulated: %d", current_encoder_value, delta, accumulated_delta);

            int steps = accumulated_delta / 4;
            if (steps != 0) {
                float step = half_step_mode ? (switching_angle / 2.0f) : switching_angle;

                int intended_angle = current_angle + (int)(steps * step);
                current_angle = clamp_f(intended_angle, 0, 180);

                if (intended_angle != current_angle) {
                    trigger_beep();
                }

                set_servo_angle_ledc(current_angle);
                accumulated_delta -= steps * 4;
                ESP_LOGI("ENCODER", "angle: %d, half_step_mode: %d", current_angle, half_step_mode);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// =====================================================================

static void IRAM_ATTR button_isr_handler(void* arg) {
    int64_t now = esp_timer_get_time();
    if (now - last_isr_time > DEBOUNCE_TIME_US) {
        half_step_mode = !half_step_mode;
        last_isr_time = now;
    }
}

static inline int clamp_f(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

void setup_encoder_pcnt(void) {
    pcnt_unit_config_t unit_config = {
        .high_limit = 10000,
        .low_limit = -10000,
        .flags.accum_count = true
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    pcnt_chan_config_t chan_config_0 = {
        .edge_gpio_num = ENCODER_GPIO_A,
        .level_gpio_num = ENCODER_GPIO_B,
    };
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_config_0, &pcnt_channel_0));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_channel_0,
        PCNT_CHANNEL_EDGE_ACTION_DECREASE,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE
    ));

    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_channel_0,
        PCNT_CHANNEL_LEVEL_ACTION_KEEP,
        PCNT_CHANNEL_LEVEL_ACTION_INVERSE
    ));

    pcnt_chan_config_t chan_config_1 = {
        .edge_gpio_num = ENCODER_GPIO_B,
        .level_gpio_num = ENCODER_GPIO_A,
    };
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_config_1, &pcnt_channel_1));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_channel_1,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE,
        PCNT_CHANNEL_EDGE_ACTION_DECREASE
    ));

    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_channel_1,
        PCNT_CHANNEL_LEVEL_ACTION_KEEP,
        PCNT_CHANNEL_LEVEL_ACTION_INVERSE
    ));

    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));
    ESP_LOGI("ENCODER", "PCNT X4 Quadrature Mode Initialized");
}

void setup_encoder_button(void) {
    gpio_config_t btn_config = {
        .pin_bit_mask = (1ULL << ENCODER_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&btn_config));

    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(ENCODER_BUTTON_GPIO, button_isr_handler, NULL));
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

void setup_buzzer(void) {
    ledc_timer_config_t buzzer_timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = BUZZER_RESOLUTION,
        .timer_num = BUZZER_TIMER,
        .freq_hz = BUZZER_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&buzzer_timer_conf));

    ledc_channel_config_t buzzer_channel_conf = {
        .gpio_num = BUZZER_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = BUZZER_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = BUZZER_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&buzzer_channel_conf));

    esp_timer_create_args_t timer_args = {
        .callback = buzzer_off_cb,
        .name = "buzzer_off"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &buzzer_timer));
}

static void buzzer_off_cb(void* arg) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);
}

void trigger_beep(void) {
    uint32_t duty = (1 << BUZZER_RESOLUTION) / 2;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);

    esp_timer_stop(buzzer_timer);
    esp_timer_start_once(buzzer_timer, BEEP_DURATION_MS * 1000);
}

void set_servo_angle_ledc(int angle) {
    angle = clamp_f(angle, 0, 180);

    float pulse_us = MAX_PULSE_US - (angle / 180.0f) * (MAX_PULSE_US - MIN_PULSE_US);
    uint32_t duty = (uint32_t)((pulse_us / PERIOD_US) * (1 << SERVO_RESOLUTION));

    ledc_set_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL);
}