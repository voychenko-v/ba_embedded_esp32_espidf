#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BUF_SIZE 1024
#define TXD_PIN 17
#define RXD_PIN 18
#define UART_BAUD_RATE 115200

static const char *TAG = "UART_APP";

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

// ---------- TX ----------
int sendData(const char *data)
{
    const int txBytes = uart_write_bytes(UART_NUM_1, data, strlen(data));
    ESP_LOGI(TAG, "Sent %d bytes", txBytes);
    return txBytes;
}

// ---------- RX ----------
int receiveData(char *buffer, int buffer_size, int timeout_ms)
{
    const int rxBytes = uart_read_bytes(UART_NUM_1, buffer, buffer_size - 1,
                                        pdMS_TO_TICKS(timeout_ms));
    if (rxBytes > 0) {
        buffer[rxBytes] = 0;
        ESP_LOGI(TAG, "Received: '%s'", buffer);
    }
    return rxBytes;
}

void app_main(void)
{
    uart_init();

    char data[BUF_SIZE];

    while (1) {
        if (receiveData(data, BUF_SIZE, 20) > 0) {
            if (strstr(data, "hello") != NULL) {
                sendData("hi from ESP32\n");
            }
        }
    }
}