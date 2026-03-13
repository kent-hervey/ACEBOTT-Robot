#define SDKCONFIG_DEPRECATED_DRIVERS_ALLOW 1
#include "acebott_hw.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/adc_types_legacy.h"
#include "driver/rmt_types_legacy.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "driver/rmt.h"
#include "freertos/ringbuf.h"
#include "esp_cpu.h"

// --- MOTOR DRIVER PINS (ESP32 Car Shield V1.0/V1.1)
#define PIN_MOTOR_EN        16
#define PIN_MOTOR_DATA      5   // Was 14 - CORRECTED for Car Shield
#define PIN_MOTOR_LATCH     17  // Was 12 - CORRECTED (12 is now free for headlight!)
#define PIN_MOTOR_CLOCK     18  // Was 15 - CORRECTED for Car Shield
#define PIN_PWM1            19
#define PIN_PWM2            23

// --- SENSOR PINS ---
#define PIN_LINE_L           35
#define PIN_LINE_M           36
#define PIN_LINE_R           39
#define PIN_ULTRASONIC_TRIG  13
#define PIN_ULTRASONIC_ECHO  27

// --- GPIO NOTES ---
// GPIO 2 is a bootstrap pin - must be LOW/floating during boot to boot from flash
// GPIO 12 is a strapping pin - can cause boot issues if held HIGH during boot

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
        .pin_bit_mask = (1ULL << PIN_MOTOR_EN) | (1ULL << PIN_MOTOR_DATA) | (1ULL << PIN_MOTOR_LATCH) | (1ULL << PIN_MOTOR_CLOCK) | (1ULL << PIN_ULTRASONIC_TRIG) | (1ULL << HEADLIGHT_L_GPIO) | (1ULL << HEADLIGHT_R_GPIO) | (1ULL << BUZZER_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&out_conf);

    // Initialize motor control pins to safe states
    gpio_set_level(PIN_MOTOR_EN, 0);      // LOW enables the motor driver
    gpio_set_level(PIN_MOTOR_DATA, 0);
    gpio_set_level(PIN_MOTOR_CLOCK, 0);
    gpio_set_level(PIN_MOTOR_LATCH, 0);

    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,  // Arduino uses 8-bit (0-255)
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 1000,  // Arduino default is ~1kHz
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t pwm1_chan = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .gpio_num = PIN_PWM1,
        .duty = 0
    };
    ledc_channel_config(&pwm1_chan);

    ledc_channel_config_t pwm2_chan = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .timer_sel = LEDC_TIMER_0,
        .gpio_num = PIN_PWM2,
        .duty = 0
    };
    ledc_channel_config(&pwm2_chan);

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_12);
    adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_12);

    rmt_config_t rmt_rx = {
        .rmt_mode = RMT_MODE_RX,
        .channel = RMT_CHANNEL_0,
        .gpio_num = IR_RECEIVER_PIN,
        .clk_div = 80,
        .mem_block_num = 2,
        .rx_config = {
            .idle_threshold = 12000,
            .filter_ticks_thresh = 100,
            .filter_en = true
        }
    };
    esp_err_t err1 = rmt_config(&rmt_rx);
    esp_err_t err2 = rmt_driver_install(rmt_rx.channel, 2000, 0);
    esp_err_t err3 = rmt_rx_start(RMT_CHANNEL_0, true);

    ESP_LOGI("IR_INIT", "RMT config: %s, install: %s, start: %s",
             esp_err_to_name(err1), esp_err_to_name(err2), esp_err_to_name(err3));
    ESP_LOGI("IR_INIT", "IR receiver initialized on GPIO %d", IR_RECEIVER_PIN);
}

void acebott_move(motor_dir_t dir, uint8_t speed) {
    // Match Arduino library exactly: digitalWrite(EN_PIN, LOW);
    gpio_set_level(PIN_MOTOR_EN, 0);

    // Match Arduino library: analogWrite(PWM1_PIN, Speed); analogWrite(PWM2_PIN, Speed);
    // Arduino analogWrite uses 8-bit (0-255), we changed to 8-bit resolution
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, speed);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, speed);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);

    // Match Arduino library: digitalWrite(STCP_PIN, LOW); shiftOut(...); digitalWrite(STCP_PIN, HIGH);
    gpio_set_level(PIN_MOTOR_LATCH, 0);

    // shiftOut(DATA_PIN, SHCP_PIN, MSBFIRST, Dir)
    for (int i = 7; i >= 0; i--) {
        gpio_set_level(PIN_MOTOR_DATA, (dir >> i) & 0x01);
        gpio_set_level(PIN_MOTOR_CLOCK, 1);
        gpio_set_level(PIN_MOTOR_CLOCK, 0);
    }

    gpio_set_level(PIN_MOTOR_LATCH, 1);

}

void acebott_read_line_sensors(int *l, int *m, int *r) {
    *l = adc1_get_raw(ADC1_CHANNEL_7);
    *m = adc1_get_raw(ADC1_CHANNEL_0);
    *r = adc1_get_raw(ADC1_CHANNEL_3);
}

ir_button_t acebott_get_ir_command(void) {
    static uint32_t last_code __attribute__((unused)) = 0;
    static bool logged_no_ringbuf = false;
    size_t length = 0;
    RingbufHandle_t rb = NULL;
    rmt_get_ringbuf_handle(RMT_CHANNEL_0, &rb);
    if (!rb) {
        if (!logged_no_ringbuf) {
            esp_rom_printf("ERROR: RMT ringbuffer is NULL!\n");
            logged_no_ringbuf = true;
        }
        return IR_CMD_NONE;
    }

    rmt_item32_t* items = (rmt_item32_t*)xRingbufferReceive(rb, &length, 0);
    if (items) {
        int num_items = length / sizeof(rmt_item32_t);

        // DEBUG: Always log when we receive IR data
        esp_rom_printf("IR: Received %d items\n", num_items);

        // Show first 3 items to understand timing structure
        if (num_items >= 3) {
            esp_rom_printf("  Item[0]: d0=%d d1=%d\n", items[0].duration0, items[0].duration1);
            esp_rom_printf("  Item[1]: d0=%d d1=%d\n", items[1].duration0, items[1].duration1);
            esp_rom_printf("  Item[2]: d0=%d d1=%d\n", items[2].duration0, items[2].duration1);
        }

        // NEC repeat code: very short (2-3 items)
        if (num_items < 10) {
            esp_rom_printf("IR: Ignoring repeat/short code (%d items)\n", num_items);
            vRingbufferReturnItem(rb, (void*)items);
            // Ignore repeat codes - return NONE so we only act on first press
            return IR_CMD_NONE;
        }

        // Full NEC code: ~68 items (34 bits × 2)
        uint32_t code = 0;
        if (num_items >= 34) {
            // NEC Protocol: LSB-first, 32 bits
            // Each bit is encoded as: MARK (560µs) + SPACE (variable)
            //   - Logical '0': MARK + 560µs space
            //   - Logical '1': MARK + 1690µs space
            // RMT items: duration1 = mark (high), duration0 = space (low)
            // Threshold for distinguishing: 1200µs (between 560 and 1690)

            for (int i = 1; i < 33 && i < num_items; i++) {
                code <<= 1;
                // NEC protocol check - try duration1 instead
                // Looks like RMT stores variable part in duration1
                if (items[i].duration1 > 1000) {
                    code |= 1;  // Long duration = logical '1'
                }
                // Short duration (~560µs) = logical '0', bit stays 0
            }
        }

        vRingbufferReturnItem(rb, (void*)items);

        // DEBUG: Always log the decoded code
        esp_rom_printf("IR: Decoded code = 0x%08X\n", code);

        if (code != 0 && code != 0xFFFFFFFF) {
            last_code = code;
            int64_t uptime_ms = esp_timer_get_time() / 1000;
            esp_rom_printf("\n");
            esp_rom_printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
            esp_rom_printf("IR SIGNAL RECEIVED - Elapsed Time: %lld ms\n", uptime_ms);
            esp_rom_printf("  Code: 0x%08X | Items: %d\n", code, num_items);

            // Debug: Show first 5 pulse timings
            esp_rom_printf("  First 5 pulses (d0=space, d1=mark):\n");
            for (int i = 0; i < 5 && i < num_items; i++) {
                esp_rom_printf("    [%d] d0=%d d1=%d\n", i, items[i].duration0, items[i].duration1);
            }

            esp_rom_printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
            return (ir_button_t)code;
        } else {
            esp_rom_printf("IR: Code filtered out (0x%08X)\n", code);
        }
    }
    return IR_CMD_NONE;
}

void acebott_beep(uint32_t freq, uint32_t duration_ms) {
    // Simple GPIO toggle for buzzer (PWM conflicts with motor driver)
    int cycles = (freq * duration_ms) / 1000;
    int half_period_us = 500000 / freq;

    for (int i = 0; i < cycles; i++) {
        gpio_set_level(BUZZER_GPIO, 1);
        esp_rom_delay_us(half_period_us);
        gpio_set_level(BUZZER_GPIO, 0);
        esp_rom_delay_us(half_period_us);
    }
}

void acebott_set_headlights(bool on) {
    gpio_set_level(HEADLIGHT_L_GPIO, on ? 1 : 0);
    gpio_set_level(HEADLIGHT_R_GPIO, on ? 1 : 0);
}

void acebott_flash_headlights(int count, uint32_t duration_ms) {
    for (int i = 0; i < count; i++) {
        acebott_set_headlights(true);
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        acebott_set_headlights(false);
        if (i < count - 1) {
            vTaskDelay(pdMS_TO_TICKS(duration_ms));
        }
    }
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