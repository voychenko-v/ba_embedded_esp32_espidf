# ADC + потенціометр (ESP32-S3, ESP-IDF)

Домашнє завдання: зчитування напруги з потенціометра через ADC у режимі `oneshot`, з порівнянням розрахункового та каліброваного значення напруги.

## Схема підключення

Потенціометр (~10 кОм) підключений як подільник напруги:
- крайній вивід 1 → **3.3V**
- крайній вивід 2 → **GND**
- середній вивід (движок) → **GPIO1** (ADC1, канал 0)

## Конфігурація ADC

| Параметр | Значення |
|---|---|
| ADC unit | ADC_UNIT_1 |
| Канал | ADC_CHANNEL_0 (GPIO1) |
| Атенюація | ADC_ATTEN_DB_12 (діапазон ~0–3.1В) |
| Розрядність | ADC_BITWIDTH_12 (0–4095) |


## Логіка вимірювання

На кожній ітерації циклу:
1. Зчитується сире значення АЦП (`adc_oneshot_read`).
2. Розраховується напруга вручну за формулою:
   ```
   V = (raw / ADC_CAPACITY) * Vref * 1000   [мВ]
   ```
3. Паралельно зчитується напруга через калібрований драйвер (`adc_cali_raw_to_voltage`), який враховує заводські eFuse-параметри чипа.
4. Обчислюється відносна похибка між розрахунковим і каліброваним значенням:
   ```
   error (%) = ((V_calc - V_cali) / V_cali) * 100
   ```

## Результати

```
I (39298) ADC: ADC1 Channel[0]: Raw: 0    | Calibration Voltage: 0    mV | Calc Voltage: 0.00    mV || Error: nan %
I (40298) ADC: ADC1 Channel[0]: Raw: 569  | Calibration Voltage: 497  mV | Calc Voltage: 458.53  mV || Error: -7.74 %
I (41298) ADC: ADC1 Channel[0]: Raw: 1071 | Calibration Voltage: 916  mV | Calc Voltage: 863.08  mV || Error: -5.78 %
I (42298) ADC: ADC1 Channel[0]: Raw: 1737 | Calibration Voltage: 1469 mV | Calc Voltage: 1399.78 mV || Error: -4.71 %
I (43298) ADC: ADC1 Channel[0]: Raw: 2424 | Calibration Voltage: 2041 mV | Calc Voltage: 1953.41 mV || Error: -4.29 %
I (44298) ADC: ADC1 Channel[0]: Raw: 3016 | Calibration Voltage: 2512 mV | Calc Voltage: 2430.48 mV || Error: -3.25 %
I (45298) ADC: ADC1 Channel[0]: Raw: 3795 | Calibration Voltage: 3019 mV | Calc Voltage: 3058.24 mV || Error:  1.30 %
I (46298) ADC: ADC1 Channel[0]: Raw: 4095 | Calibration Voltage: 3152 mV | Calc Voltage: 3300.00 mV || Error:  4.70 %
```
