#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

/* ---------- Налаштування ---------- */
#define BUTTON_PIN         GPIO_NUM_2
#define POLL_INTERVAL_MS   10
#define DEBOUNCE_TIME_MS   50

static const char *TAG = "button_fsm";

/* ---------- Типи ---------- */
typedef enum {
    BTN_IDLE = 0,
    BTN_PRESS_DEBOUNCE,
    BTN_PRESSED,
    BTN_RELEASE_DEBOUNCE
} button_state_t;

typedef struct {
    button_state_t state;
    int64_t        state_enter_tick;   /* мс, від esp_timer_get_time() */
    bool           event_pressed;
    bool           event_released;
} button_fsm_t;

/* ---------- Глобальний стан ---------- */
static button_fsm_t btn = {
    .state             = BTN_IDLE,
    .state_enter_tick  = 0,
    .event_pressed     = false,
    .event_released    = false,
};

static uint16_t pressed_count  = 0;
static uint16_t released_count = 0;

/* ---------- Прототипи ---------- */
static void button_gpio_init(void);
static void button_fsm_update(void);
static bool button_was_just_pressed(void);
static bool button_was_just_released(void);

/* ---------- Ініціалізація GPIO ---------- */
static void button_gpio_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
}


static void button_fsm_update(void)
{
    int64_t now_ms = esp_timer_get_time() / 1000; /* esp_timer -> мікросекунди */
    int pin_state = gpio_get_level(BUTTON_PIN);

    switch (btn.state) {
    case BTN_IDLE:
        if (pin_state == 1) {
            btn.state = BTN_PRESS_DEBOUNCE;
            btn.state_enter_tick = now_ms;
        }
        break;

    case BTN_PRESS_DEBOUNCE:
        if (pin_state == 0) {
            btn.state = BTN_IDLE;
        } else if ((now_ms - btn.state_enter_tick) >= DEBOUNCE_TIME_MS) {
            btn.state = BTN_PRESSED;
            btn.event_pressed = true;
        }
        break;

    case BTN_PRESSED:
        if (pin_state == 0) {
            btn.state = BTN_RELEASE_DEBOUNCE;
            btn.state_enter_tick = now_ms;
        }
        break;

    case BTN_RELEASE_DEBOUNCE:
        if (pin_state == 1) {
            btn.state = BTN_PRESSED;
        } else if ((now_ms - btn.state_enter_tick) >= DEBOUNCE_TIME_MS) {
            btn.state = BTN_IDLE;
            btn.event_released = true;
        }
        break;
    }
}


static bool button_was_just_pressed(void)
{
    if (btn.event_pressed) {
        btn.event_pressed = false;
        return true;
    }
    return false;
}

static bool button_was_just_released(void)
{
    if (btn.event_released) {
        btn.event_released = false;
        return true;
    }
    return false;
}


void app_main(void)
{
    button_gpio_init();

    while (1) {
        button_fsm_update();

        if (button_was_just_pressed()) {
            pressed_count++;
            ESP_LOGI(TAG, "Button pressed: %d", pressed_count);
        }
        if (button_was_just_released()) {
            released_count++;
            ESP_LOGI(TAG, "Button released: %d", released_count);
        }

        vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL_MS));
    }
}