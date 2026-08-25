#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h" 
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define LED_GREEN_PIN  GPIO_NUM_38
#define LED_YELLOW_PIN GPIO_NUM_39
#define LED_RED_PIN    GPIO_NUM_40
#define BUTTON_PIN     GPIO_NUM_41
#define LED_ACTIV_TIME 1500
#define BLINK_INTERVAL 250
#define DEBOUNCE_MS    60

static int64_t last_press_time = 0;
static bool yellow_blink_on = false;
static QueueHandle_t button_evt_queue  = NULL;

static void IRAM_ATTR button_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(button_evt_queue, &gpio_num, NULL);
}

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
    .pull_down_en = GPIO_PULLDOWN_ENABLE,
    .intr_type = GPIO_INTR_POSEDGE,
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

        case GREEN: {
            if (now_ms - led_current.last_swich_time >= LED_ACTIV_TIME) {
                led_current.state = GREEN_BLINK;
                led_current.last_swich_time = now_ms;
            }
            break;
        }
        case GREEN_BLINK: {
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
        }
        case YELLOW: {
            if (now_ms - led_current.last_swich_time >= LED_ACTIV_TIME) {
                gpio_set_level(LED_YELLOW_PIN, 0);
                gpio_set_level(LED_RED_PIN, 1);
                led_current.state = RED;
                led_current.last_swich_time = now_ms;
            }
            break;
        }
        case RED: {
            if (now_ms - led_current.last_swich_time >= LED_ACTIV_TIME) {
                gpio_set_level(LED_YELLOW_PIN, 1);
                led_current.state = RED_YELLOW;
                led_current.last_swich_time = now_ms;
            }
            break;
        }
        case RED_YELLOW: {
            if (now_ms - led_current.last_swich_time >= LED_ACTIV_TIME) {
                gpio_set_level(LED_YELLOW_PIN, 0);
                gpio_set_level(LED_RED_PIN, 0);
                gpio_set_level(LED_GREEN_PIN, 1);
                led_current.state = GREEN;
                led_current.last_swich_time = now_ms;
            }
            break;
        }
        case YELLOW_BLINK: {
            int64_t elapsed = now_ms - led_current.last_swich_time;
            bool led_on = (elapsed / BLINK_INTERVAL) % 2 == 0;
            gpio_set_level(LED_YELLOW_PIN, led_on);
            break;
        }
    }

}

void app_main(void)
{
    gpio_config(&io_conf);
    gpio_config(&btn_conf);
    gpio_set_level(LED_GREEN_PIN, 1);

    // Переривання
    button_evt_queue = xQueueCreate(1, sizeof(uint32_t));
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, (void *)BUTTON_PIN);

    while (1) {
        
        traffic_lights_fsm();

        uint32_t io_num;
        if (xQueueReceive(button_evt_queue, &io_num, 0)) {
            while (xQueueReceive(button_evt_queue, &io_num, 0)) {} // Видаляємо події, спричинені брязкотом

            int64_t now = esp_timer_get_time() / 1000;
            if (now - last_press_time >= DEBOUNCE_MS && gpio_get_level(BUTTON_PIN) == 1) {
                last_press_time = now;

                gpio_set_level(LED_GREEN_PIN, 0);
                gpio_set_level(LED_YELLOW_PIN, 0);
                gpio_set_level(LED_RED_PIN, 0);

                yellow_blink_on = !yellow_blink_on;
                led_current.state = yellow_blink_on ? YELLOW_BLINK : GREEN;
                gpio_set_level(LED_GREEN_PIN, !yellow_blink_on);
                led_current.last_swich_time = now;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}