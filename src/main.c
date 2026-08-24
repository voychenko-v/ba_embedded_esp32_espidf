#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define LED_GREEN_PIN  GPIO_NUM_38
#define LED_YELLOW_PIN GPIO_NUM_39
#define LED_RED_PIN    GPIO_NUM_40
#define BUTTON_PIN     GPIO_NUM_41
#define LED_ACTIV_TIME 2000
#define BLINK_INTERVAL 250   


gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << LED_GREEN_PIN) | (1ULL << LED_YELLOW_PIN)| (1ULL << LED_RED_PIN),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
};

gpio_config_t btn_conf = {
    .pin_bit_mask = (1ULL << BUTTON_PIN),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
};

typedef enum {
    GREEN=0,
    GREEN_BLINK,
    YELLOW,
    RED,
    RED_YELLOW,
    YELLOW_BLINK
} led_state_t;

typedef struct {
    led_state_t state;
    int64_t last_swich_time;
} traffic_lights_t;

traffic_lights_t led_current = {
    .state = GREEN,
    .last_swich_time = 0
};


void traffic_lights_fsm(void)
{
    int64_t now_ms = esp_timer_get_time() / 1000;

    switch (led_current.state) {

    case GREEN:
        if (now_ms - led_current.last_swich_time >= LED_ACTIV_TIME) {
            led_current.state = GREEN_BLINK;
            led_current.last_swich_time = now_ms;
            gpio_set_level(LED_GREEN_PIN, 1);
        }
        break;

    case GREEN_BLINK:
        if (now_ms - led_current.last_swich_time >= LED_ACTIV_TIME) {
            gpio_set_level(LED_GREEN_PIN, 0);
            gpio_set_level(LED_YELLOW_PIN, 1);
            led_current.state = YELLOW;
            led_current.last_swich_time = now_ms;
            break;
        }

        int64_t elapsed = now_ms - led_current.last_swich_time;
        bool led_on = (elapsed / BLINK_INTERVAL) % 2 == 0;
        gpio_set_level(LED_GREEN_PIN, led_on);
        break;

    case YELLOW:
        if (now_ms - led_current.last_swich_time >= LED_ACTIV_TIME) {
            gpio_set_level(LED_YELLOW_PIN, 0);
            gpio_set_level(LED_RED_PIN, 1);
            led_current.state = RED;
            led_current.last_swich_time = now_ms;
        }
        break;

    case RED:
        if (now_ms - led_current.last_swich_time >= LED_ACTIV_TIME) {
            gpio_set_level(LED_RED_PIN, 0);
            gpio_set_level(LED_GREEN_PIN, 1);
            led_current.state = GREEN;
            led_current.last_swich_time = now_ms;
        }
        break;
    }
}

void app_main(void)
{
    gpio_config(&io_conf);
    gpio_config(&btn_conf);

    while (1) {
        traffic_lights_fsm();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}