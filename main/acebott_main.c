#include "acebott_hw.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ACEBOTT_BRAIN";

void app_main(void) {
    acebott_init();
    ESP_LOGI(TAG, "Acebott System Online. Line Tracking & IR Ready.");
    acebott_beep(1000, 200);

    int left_val, mid_val, right_val;
    int black_line_threshold = 2000; // Adjust based on your floor/tape
    uint8_t current_speed = 120;

    while (1) {
        // --- Section 1: Safety Veto (Ultrasonic) ---
        float dist = acebott_get_distance();
        if (dist > 0 && dist < 15.0f) {
            acebott_move(MOTOR_STOP, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue; // Skip IR and Line logic if we're about to crash
        }

        // --- Section 2: Remote Override ---
        ir_button_t cmd = acebott_get_ir_command();
        if (cmd != IR_CMD_NONE) {
            switch (cmd) {
                case IR_CMD_UP:    acebott_move(MOTOR_FORWARD, 150); break;
                case IR_CMD_DOWN:  acebott_move(MOTOR_BACK, 150);    break;
                case IR_CMD_STOP:  acebott_move(MOTOR_STOP, 0);      break;
                default: break;
            }
        }
        else {
            // --- Section 3: The Road Thingy Logic (Line Following) ---
            // Only runs if no IR button is being pressed
            acebott_read_line_sensors(&left_val, &mid_val, &right_val);

            // Simple Logic: If Middle is Black (>= threshold), go straight
            if (mid_val >= black_line_threshold) {
                acebott_move(MOTOR_FORWARD, current_speed);
            }
            // If Left is Black, nudge Left
            else if (left_val >= black_line_threshold) {
                acebott_move(MOTOR_CCW, 100);
            }
            // If Right is Black, nudge Right
            else if (right_val >= black_line_threshold) {
                acebott_move(MOTOR_CW, 100);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20)); // High-speed 50Hz polling for smooth tracking
    }
}