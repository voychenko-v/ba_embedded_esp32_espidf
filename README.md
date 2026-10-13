# ESP32-S3 + ESP-IDF: навчальні проєкти

Практичні завдання з вивчення ESP32-S3 (N16R8) на фреймворку **ESP-IDF** у середовищі **PlatformIO**: від дебаунсу кнопки до обміну даними з STM32 по UART.

Кожне завдання лежить в окремій гілці: код, README зі схемою підключення та, де є, демонстрація (GIF).

## Стек

- **МК:** ESP32-S3 DevKitC-1 (N16R8)
- **Фреймворк:** ESP-IDF (C)
- **Збірка:** PlatformIO
- **Периферія та підходи:** GPIO, переривання, ADC (oneshot + калібрування), LEDC (PWM), PCNT, `esp_timer`, UART, скінченні автомати

## Завдання

| № | Гілка | Тема | Що відпрацьовано |
|---|---|---|---|
| 01 | [`01_FSM_debounce`](https://github.com/voychenko-v/ba_embedded_esp32_espidf/tree/01_FSM_debounce) | Дебаунс кнопки | Опитування кожні 10 мс, дебаунс як скінченний автомат (`IDLE → PRESS_DEBOUNCE → PRESSED → RELEASE_DEBOUNCE`) |
| 02 | [`02_traffic_lights`](https://github.com/voychenko-v/ba_embedded_esp32_espidf/tree/02_traffic_lights) | Світлофор | FSM режимів, GPIO-переривання для спеціального режиму (жовтий миготливий) |
| 03 | [`03_adc_calibration`](https://github.com/voychenko-v/ba_embedded_esp32_espidf/tree/03_adc_calibration) | ADC + потенціометр | `adc_oneshot`, калібрування через eFuse, порівняння розрахункової та каліброваної напруги, оцінка похибки |
| 04 | [`04_simple_moving_average`](https://github.com/voychenko-v/ba_embedded_esp32_espidf/tree/04_simple_moving_average) | Фоторезистор + SMA | Ковзне середнє для згладжування шуму АЦП, гістерезис, FSM керування світлодіодом |
| 05 | [`05_LEDS_PWM`](https://github.com/voychenko-v/ba_embedded_esp32_espidf/tree/05_LEDS_PWM) | LED і мотор через PWM | LEDC: окремі таймери та канали, один потенціометр керує яскравістю та швидкістю |
| 06 | [`06_buzzer`](https://github.com/voychenko-v/ba_embedded_esp32_espidf/tree/06_buzzer) | Мелодія на зумері | PWM із фіксованою несучою + таблиця синуса в таймерному перериванні (`esp_timer`) |
| 07 | [`07_servo`](https://github.com/voychenko-v/ba_embedded_esp32_espidf/tree/07_servo) | Сервопривід SG90 | PWM 50 Гц, підбір діапазону імпульсу (500-2500 мкс), кут з потенціометра |
| 08 | [`08_refactor`](https://github.com/voychenko-v/ba_embedded_esp32_espidf/tree/08_refactor) | Рефакторинг | ADC → SMA → калібрування (`curve_fitting`) → кут → PWM duty, логування проміжних значень |
| 09 | [`09_control_servo_using_encoder-`](https://github.com/voychenko-v/ba_embedded_esp32_espidf/tree/09_control_servo_using_encoder-) | Енкодер + сервопривід | Квадратурний енкодер на апаратному `PCNT`, переривання кнопки, зумер на межах діапазону |
| 10 | [`10_UART`](https://github.com/voychenko-v/ba_embedded_esp32_espidf/tree/10_UART) | **ESP32-S3 ↔ STM32** | Обмін повідомленнями між двома МК по UART, дебаунс, прийом до `\n` |

## Пов'язані репозиторії

- [STM32_F401CCU6](https://github.com/voychenko-v/STM32_F401CCU6): проєкти на STM32F401CCU6 (HAL), зокрема метеостанція-годинник на BME280 (SPI), SSD1306 та DS1307 (I2C). Кожне завдання лежить в окремій гілці

