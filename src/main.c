#include <math.h>
#include "driver/ledc.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BUZZER_PIN       18
#define CARRIER_FREQ_HZ  1000
#define NUM_SAMPLES      112

#define NOTE_C4  262
#define NOTE_G4  392
#define NOTE_A4  440
#define REST     0

static uint8_t sineTable[NUM_SAMPLES];
static volatile int sampleIndex = 0;
static volatile float frequencyMultiplier = 0.0;

static void IRAM_ATTR pwm_isr(void *arg) {
    sampleIndex += (int)frequencyMultiplier;
    if (sampleIndex >= NUM_SAMPLES) sampleIndex -= NUM_SAMPLES;

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, sineTable[sampleIndex]);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}


void playNote(int frequency, int durationMs) {
    frequencyMultiplier = (frequency == 0) ? 0.0 : (float)(frequency * NUM_SAMPLES) / CARRIER_FREQ_HZ;
    vTaskDelay(pdMS_TO_TICKS(durationMs));

    frequencyMultiplier = 0.0;

    ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0); 
    vTaskDelay(pdMS_TO_TICKS(50));

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 128);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

static const int melody[][2] = {
    {NOTE_C4, 400},
    {NOTE_C4, 400},
    {NOTE_G4, 400},
    {NOTE_G4, 400},
    {NOTE_A4, 400},
    {NOTE_A4, 400},
    {NOTE_G4, 800},
};
#define MELODY_LEN (sizeof(melody) / sizeof(melody[0]))

void app_main(void) {

    for (int i = 0; i < NUM_SAMPLES; i++)
        sineTable[i] = 128 + 127 * sinf(2 * M_PI * i / NUM_SAMPLES);

    ledc_timer_config_t timer_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = CARRIER_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_cfg);

    ledc_channel_config_t channel_cfg = {
        .gpio_num = BUZZER_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 128,
        .hpoint = 0
    };
    ledc_channel_config(&channel_cfg);

    // Таймер переривання на частоту несучої
    const esp_timer_create_args_t timer_args = {
        .callback = &pwm_isr,
        .name = "pwm_isr"
    };
    esp_timer_handle_t sample_timer;
    esp_timer_create(&timer_args, &sample_timer);
    esp_timer_start_periodic(sample_timer, 1000000 / CARRIER_FREQ_HZ);

    while (1) {
        for (int i = 0; i < MELODY_LEN; i++) {
            playNote(melody[i][0], melody[i][1]);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}