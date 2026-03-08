//
// Created by Kent Hervey on 3/6/26.
//
#include "acebott_hw.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h" // Needed for the line sensors

// Pin Definitions from Acebott schematics
#define PIN_MOTOR_EN     16
#define PIN_MOTOR_DATA    5
#define PIN_MOTOR_LATCH  17
#define PIN_MOTOR_CLOCK  18
#define PIN_BUZZER       33
#define PIN_PWM1         19
#define PIN_PWM2         23
// Add these to acebott_hw.c
#define PIN_LINE_L 35
#define PIN_LINE_M 36
#define PIN_LINE_R 39

void acebott_init(void) {
    // GPIO Config for Shift Register
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL<<PIN_MOTOR_EN) | (1ULL<<PIN_MOTOR_DATA) |
                        (1ULL<<PIN_MOTOR_LATCH) | (1ULL<<PIN_MOTOR_CLOCK),
        .mode = GPIO_MODE_OUTPUT
    };
    gpio_config(&io_conf);
    gpio_set_level(PIN_MOTOR_EN, 1); // Start Disabled

    // PWM Config for Buzzer & Motors
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_conf);
}

void acebott_move(motor_dir_t dir, uint8_t speed) {
    // 1. Bit-bang the 74HC595
    gpio_set_level(PIN_MOTOR_LATCH, 0);
    for (int i = 0; i < 8; i++) {
        gpio_set_level(PIN_MOTOR_DATA, (dir >> (7 - i)) & 1);
        gpio_set_level(PIN_MOTOR_CLOCK, 1);
        esp_rom_delay_us(1);
        gpio_set_level(PIN_MOTOR_CLOCK, 0);
    }
    gpio_set_level(PIN_MOTOR_LATCH, 1);

    // 2. Enable Motors
    gpio_set_level(PIN_MOTOR_EN, 0);
    // (PWM speed logic would be set here via ledc_set_duty)
}

void acebott_beep(uint32_t freq, uint32_t duration_ms) {
    // Setup PWM frequency for the buzzer
    ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 4096); // 50% duty
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    vTaskDelay(pdMS_TO_TICKS(duration_ms));

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0); // Silence
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

// Temporary placeholder for IR until we add the RMT driver
ir_button_t acebott_get_ir_command(void) {
    return IR_CMD_NONE;
}

// Read the "Road Thingy" (Analog sensors)
void acebott_read_line_sensors(int *l, int *m, int *r) {
    // We use the pins defined in the Acebott spec
    // ADC1_CHANNEL_7 is GPIO 35, CHANNEL_0 is GPIO 36, CHANNEL_3 is GPIO 39
    *l = adc1_get_raw(ADC1_CHANNEL_7); // Left (GPIO 35)
    *m = adc1_get_raw(ADC1_CHANNEL_0); // Mid  (GPIO 36)
    *r = adc1_get_raw(ADC1_CHANNEL_3); // Right(GPIO 39)
}