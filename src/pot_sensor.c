#include "pot_sensor.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"

#define ADC_ATTEN ADC_ATTEN_DB_12
#define ADC_BITWIDTH ADC_BITWIDTH_12
#define ADC_CAPACITY ((1 << ADC_BITWIDTH) - 1)
#define POT_ADC_CHANNEL ADC_CHANNEL_3


static adc_oneshot_unit_handle_t adc1_handle;

static long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void pot_sensor_init(void){
    
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
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

uint16_t pot_sensor_read_scaled(uint16_t out_max) {
    return (uint16_t)map(pot_sensor_read_raw(), 0, ADC_CAPACITY, 0, out_max);
}