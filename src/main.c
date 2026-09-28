#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "driver/gpio.h"

#define BUTTON_PIN        GPIO_NUM_0
#define DEBOUNCE_MS       30

#define LED_PIN  GPIO_NUM_4

#define BUF_SIZE 1024
#define TXD_PIN 17
#define RXD_PIN 18
#define UART_BAUD_RATE 115200

static bool led_on = false;

static const char *TAG = "UART_APP";

static void led_init(void)
{
    const gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(LED_PIN, 0);
}

static void button_init(void)
{
    const gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
}

void uart_init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_driver_install(UART_NUM_1, BUF_SIZE, BUF_SIZE, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

static bool button_pressed_event(void)
{
    static int last_raw = 1;
    static int stable = 1;
    static int64_t last_change_ms = 0;

    int raw = gpio_get_level(BUTTON_PIN);
    int64_t now_ms = esp_timer_get_time() / 1000;

    if (raw != last_raw) {
        last_raw = raw;
        last_change_ms = now_ms;
    }

    if ((now_ms - last_change_ms) >= DEBOUNCE_MS && raw != stable) {
        stable = raw;
        if (stable == 0) {
            return true;
        }
    }
    return false;
}

static void led_toggle(void)
{
    led_on = !led_on;
    gpio_set_level(LED_PIN, led_on);
    ESP_LOGI(TAG, "LED %s", led_on ? "ON" : "OFF");
}

int sendData(const char *data)
{
    const int txBytes = uart_write_bytes(UART_NUM_1, data, strlen(data));
    ESP_LOGI(TAG, "Sent %d bytes", txBytes);
    return txBytes;
}

int receiveData(char *buffer, int buffer_size, int timeout_ms)
{
    int rxBytes = uart_read_bytes(UART_NUM_1, buffer, buffer_size - 1, pdMS_TO_TICKS(timeout_ms));
    if (rxBytes > 0) {
        buffer[rxBytes] = 0;
        int n = rxBytes;
        while (n > 0 && (buffer[n-1] == '\n' || buffer[n-1] == '\r')) n--;
        ESP_LOGI(TAG, "Received: '%.*s'", n, buffer);
    }
    return rxBytes;
}

void app_main(void)
{
    uart_init();
    button_init();
    led_init();

    char data[BUF_SIZE];

    while (1) {
        if (receiveData(data, BUF_SIZE, 20) > 0) {
            if (strstr(data, "hello") != NULL) {
                led_toggle();
            }
        }

        if (button_pressed_event()) {
            ESP_LOGI(TAG, "BOOT button pressed");
            sendData("hello\n");
        }
    }
}