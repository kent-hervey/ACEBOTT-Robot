#include "acebott_hw.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//for blinking testing
#include "driver/gpio.h"      // For gpio_set_level

#define ENABLE_ULTRASONIC 0  // Set to 1 when you plug in the sensor!

static const char *TAG = "ACEBOTT_BRAIN";

void app_main(void) {
    acebott_init();
    ESP_LOGI(TAG, "Acebott System Online. Line Tracking & IR Ready.");
    acebott_beep(1000, 200);

    // Initializing variables to 0 to keep the compiler happy
    int left_val = 0, mid_val = 0, right_val = 0;
    int current_speed = 0;
    motor_dir_t current_dir = MOTOR_STOP;
    const int speed_step = 40;                 // 6 "gears" to reach max speed of 255

    // Diagnostic LED Setup
    gpio_reset_pin(2);
    gpio_set_direction(2, GPIO_MODE_OUTPUT);

    while (1) {
        // 1. INPUTS
        ir_button_t cmd = acebott_get_ir_command();
        acebott_read_line_sensors(&left_val, &mid_val, &right_val);

#if ENABLE_ULTRASONIC
        float dist = acebott_get_distance();
#else
        float dist = 0.0f;
#endif

        // 2. DIAGNOSTICS (Heartbeat LED)
        gpio_set_level(2, 1);
        ESP_LOGI(TAG, "STATS | Dist: %.1f | Line: %d-%d-%d | Speed: %d | Dir: %d",
                 dist, left_val, mid_val, right_val, current_speed, current_dir);
        vTaskDelay(pdMS_TO_TICKS(25)); // LED ON time
        gpio_set_level(2, 0);

        // 3. SMART SAFETY VETO
        bool obstacle_stop = false;
#if ENABLE_ULTRASONIC
        if (dist > 0 && dist < 15.0f && current_dir == MOTOR_FORWARD) {
            acebott_move(MOTOR_STOP, 0);
            obstacle_stop = true;
        }
#endif

        // 4. REMOTE OVERRIDE (The Acceleration Logic)
        if (cmd != IR_CMD_NONE) {
            switch (cmd) {
                case IR_CMD_UP:
                    if (current_dir == MOTOR_BACK) {
                        if (current_speed >= speed_step) current_speed -= speed_step;
                        else { current_speed = 0; current_dir = MOTOR_STOP; }
                    } else {
                        current_dir = MOTOR_FORWARD;
                        current_speed = (current_speed + speed_step > 255) ? 255 : current_speed + speed_step;
                    }
                break;
                case IR_CMD_DOWN:
                    if (current_dir == MOTOR_FORWARD) {
                        if (current_speed >= speed_step) current_speed -= speed_step;
                        else { current_speed = 0; current_dir = MOTOR_STOP; }
                    } else {
                        current_dir = MOTOR_BACK;
                        current_speed = (current_speed + speed_step > 255) ? 255 : current_speed + speed_step;
                    }
                break;
                case IR_CMD_STOP:
                    current_speed = 0;
                current_dir = MOTOR_STOP;
                break;
                default: break;
            }
            acebott_move(current_dir, current_speed);
        }
        // 5. ROAD LOGIC (Line Following)
        else if (current_dir != MOTOR_STOP && !obstacle_stop) {
            int black_line_threshold = 2000;
            if (mid_val >= black_line_threshold) {
                acebott_move(MOTOR_FORWARD, current_speed);
            } else if (left_val >= black_line_threshold) {
                acebott_move(MOTOR_CCW, 100);
            } else if (right_val >= black_line_threshold) {
                acebott_move(MOTOR_CW, 100);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(25)); // LED OFF time (Total loop = 50ms)
    }
}
