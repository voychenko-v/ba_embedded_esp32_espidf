#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"

#include "sma_filter.h"

const static char *TAG = "SMA";

#define ADC1_PIN1 ADC_CHANNEL_0
#define ADC_BITWIDTH ADC_BITWIDTH_12
#define ADC_CAPACITY ((1 << ADC_BITWIDTH) - 1)

#define LED_PIN GPIO_NUM_2

#define WINDOW_SIZE  5
#define SWITCH_VALUE 1500
#define HYSTERESIS 200

static int adc_raw = 0;

typedef enum {
    LED_OFF = 0,
    LED_ON = 1
} led_light;

static led_light led_state = LED_OFF;

gpio_config_t pin_conf = {
    .pin_bit_mask = (1ULL << LED_PIN),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
};

//-------------ADC1 Init---------------//
adc_oneshot_unit_handle_t adc1_handle;
adc_oneshot_unit_init_cfg_t init_config1 = {
    .unit_id = ADC_UNIT_1,
};

//-------------ADC1 Config---------------//
adc_oneshot_chan_cfg_t config = {
    .atten = ADC_ATTEN_DB_12,
    .bitwidth = ADC_BITWIDTH,
};

void app_main(void)
{

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC1_PIN1, &config));
    ESP_ERROR_CHECK(gpio_config(&pin_conf));

    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC1_PIN1, &adc_raw));
    static int buffer[WINDOW_SIZE];
    sma_filter_t filter;
    sma_filter_init(&filter, buffer, WINDOW_SIZE, adc_raw);

    while (1) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC1_PIN1, &adc_raw));
        int sma = sma_filter_update(&filter, adc_raw);
        
        switch (led_state)
        {
            case LED_OFF:
                if (sma < SWITCH_VALUE - HYSTERESIS) {
                    gpio_set_level(LED_PIN, 1);
                    led_state = LED_ON;
                }
                break;
            case LED_ON:
                if (sma > SWITCH_VALUE + HYSTERESIS) {
                    gpio_set_level(LED_PIN, 0);
                    led_state = LED_OFF;
                }
                break;
        }
        
        ESP_LOGI(TAG, "ADC%d Channel[%d]: Raw: %d | LED: %d" , ADC_UNIT_1 + 1, ADC1_PIN1, adc_raw, led_state);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

}
