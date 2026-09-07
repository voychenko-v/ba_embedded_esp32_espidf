#pragma once
#include <stdint.h>

void pot_sensor_init(void);
uint16_t pot_sensor_read_raw(void);
uint16_t pot_sensor_read_scaled(uint16_t out_max);
