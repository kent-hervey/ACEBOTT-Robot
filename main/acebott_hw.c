#define SDKCONFIG_DEPRECATED_DRIVERS_ALLOW 1
#include "acebott_hw.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/adc_types_legacy.h"
#include "driver/rmt_types_legacy.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "driver/rmt.h"
#include "freertos/ringbuf.h"
#include "esp_cpu.h"

#define PIN_MOTOR_EN        16
#define PIN_MOTOR_DATA      14
#define PIN_MOTOR_LATCH     12
#define PIN_MOTOR_CLOCK     15
#define PIN_PWM1            19
#define PIN_BUZZER          33
#define PIN_LINE_L          35
#define PIN_LINE_M          36
#define PIN_LINE_R          39
#define PIN_IR_RX           25
#define PIN_ULTRASONIC_TRIG 13
#define PIN_ULTRASONIC_ECHO 27

void acebott_init(void) {
    gpio_config_t echo_conf = {
        .pin_bit_mask = (1ULL << PIN_ULTRASONIC_ECHO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&echo_conf);

    gpio_config_t out_conf = {
        .pin_bit_mask = (1ULL << PIN_MOTOR_EN) | (1ULL << PIN_MOTOR_DATA) | (1ULL << PIN_MOTOR_LATCH) | (1ULL << PIN_MOTOR_CLOCK) | (1ULL << PIN_ULTRASONIC_TRIG),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&out_conf);

    gpio_set_level(PIN_MOTOR_EN, 1);

    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t pwm_chan = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .gpio_num = PIN_PWM1,
        .duty = 0
    };
    ledc_channel_config(&pwm_chan);

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_12);
    adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_12);

    rmt_config_t rmt_rx = {
        .rmt_mode = RMT_MODE_RX, .channel = RMT_CHANNEL_0,
        .gpio_num = PIN_IR_RX, .clk_div = 80, .mem_block_num = 1
    };
    rmt_config(&rmt_rx);
    rmt_driver_install(rmt_rx.channel, 1000, 0);
}

void acebott_move(motor_dir_t dir, uint8_t speed) {
    gpio_set_level(PIN_MOTOR_LATCH, 0);
    for (int i = 0; i < 8; i++) {
        gpio_set_level(PIN_MOTOR_DATA, (dir >> (7 - i)) & 1);
        gpio_set_level(PIN_MOTOR_CLOCK, 1); esp_rom_delay_us(1);
        gpio_set_level(PIN_MOTOR_CLOCK, 0);
    }
    gpio_set_level(PIN_MOTOR_LATCH, 1);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, (speed * 8191) / 255);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    gpio_set_level(PIN_MOTOR_EN, 0);
}

void acebott_read_line_sensors(int *l, int *m, int *r) {
    *l = adc1_get_raw(ADC1_CHANNEL_7);
    *m = adc1_get_raw(ADC1_CHANNEL_0);
    *r = adc1_get_raw(ADC1_CHANNEL_3);
}

ir_button_t acebott_get_ir_command(void) {
    size_t length = 0;
    RingbufHandle_t rb = NULL;
    rmt_get_ringbuf_handle(RMT_CHANNEL_0, &rb);
    if (!rb) return IR_CMD_NONE;
    rmt_item32_t* items = (rmt_item32_t*)xRingbufferReceive(rb, &length, 0);
    if (items) {
        uint32_t cmd = items[0].val;
        vRingbufferReturnItem(rb, (void*)items);
        return (ir_button_t)cmd;
    }
    return IR_CMD_NONE;
}

void acebott_beep(uint32_t freq, uint32_t duration_ms) {
    ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 4096);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

// MATH: Pure conversion function
float distance_cm_from_us(int64_t duration_us) {
    if (duration_us <= 0) return -1.0f;
    return ((float)duration_us * 0.0343f) / 2.0f;
}

// HARDWARE: Trigger sensor and read echo
float acebott_get_distance(void) {
    gpio_set_level(PIN_ULTRASONIC_TRIG, 0); esp_rom_delay_us(2);
    gpio_set_level(PIN_ULTRASONIC_TRIG, 1); esp_rom_delay_us(10);
    gpio_set_level(PIN_ULTRASONIC_TRIG, 0);

    while (gpio_get_level(PIN_ULTRASONIC_ECHO) == 0);
    int64_t start_time = esp_timer_get_time();

    while (gpio_get_level(PIN_ULTRASONIC_ECHO) == 1) {
        if ((esp_timer_get_time() - start_time) > 30000) break;
    }

    int64_t duration = (esp_timer_get_time() - start_time);
    return distance_cm_from_us(duration);
}