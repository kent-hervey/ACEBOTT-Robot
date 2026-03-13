#include "acebott_hw.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/touch_pad.h"
#include "acebott_hw.h"

//for blinking testing
#include <esp_timer.h>
#include <driver/touch_sensor.h>

#include "driver/gpio.h"      // For gpio_set_level

#define ENABLE_ULTRASONIC 0  // Set to 1 when you plug in the sensor!


static const char *TAG = "ACEBOTT_BRAIN";

void app_main(void) {

    // CRITICAL: Initialize hardware FIRST before using any motors!
    acebott_init();
    ESP_LOGI(TAG, "Acebott System Online. Line Tracking & IR Ready.");

    touch_pad_init();
    touch_pad_config(TOUCH_PAD_NUM7, 0); // GPIO 27

    // --- STARTUP SEQUENCE: Headlights/buzzer already initialized in acebott_init() ---

    // Loop GPIO test 1 time (user request: "only once")
    for (int loop = 0; loop < 1; loop++) {
        // Three beeps BEFORE GPIO test
        ESP_LOGI(TAG, "GPIO test round %d - 3 beeps BEFORE", loop + 1);
        acebott_beep(2000, 100);
        vTaskDelay(pdMS_TO_TICKS(300));
        acebott_beep(2000, 100);
        vTaskDelay(pdMS_TO_TICKS(300));
        acebott_beep(2000, 100);
        vTaskDelay(pdMS_TO_TICKS(500));

        // Test headlights
        ESP_LOGI(TAG, "  >>> Headlights going HIGH <<<");
        acebott_set_headlights(true);
        vTaskDelay(pdMS_TO_TICKS(3000));  // Hold HIGH for 3 seconds

        ESP_LOGI(TAG, "  >>> Headlights going LOW <<<");
        acebott_set_headlights(false);
        vTaskDelay(pdMS_TO_TICKS(1000));  // Hold LOW for 1 second

        // Three beeps AFTER GPIO test
        ESP_LOGI(TAG, "GPIO test round %d - 3 beeps AFTER", loop + 1);
        acebott_beep(2500, 100);
        vTaskDelay(pdMS_TO_TICKS(300));
        acebott_beep(2500, 100);
        vTaskDelay(pdMS_TO_TICKS(300));
        acebott_beep(2500, 100);
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    // Brief wheel test
    ESP_LOGI(TAG, "Testing motors (direction=%d, speed=150)...", MOTOR_FORWARD);
    acebott_move(MOTOR_FORWARD, 150);
    vTaskDelay(pdMS_TO_TICKS(500));
    acebott_move(MOTOR_STOP, 0);
    ESP_LOGI(TAG, "Motors stopped");

    // Turn on headlights after startup
    acebott_set_headlights(true);

    ESP_LOGI(TAG, "ACEBOTT QD001 startup complete! Waiting for IR commands...");

    // Initializing variables to 0 to keep the compiler happy
    int left_val = 0, mid_val = 0, right_val = 0;
    int current_speed = 0;
    motor_dir_t current_dir = MOTOR_STOP;
    const int speed_step __attribute__((unused)) = 40;  // 6 "gears" to reach max speed of 255

    // Removed diagnostic LED setup - GPIO 2 is HEADLIGHT_R_GPIO!

    while (1) {


        // --- READ INPUTS ---
        uint16_t touch_value;
        touch_pad_read(TOUCH_PAD_NUM7, &touch_value);

        // Get the real IR command
        ir_button_t cmd = acebott_get_ir_command();

        // Use touch to FAKE the IR command if touch is detected
        if (touch_value < 500 && touch_value > 0) {
            ESP_LOGI("TOUCH", "Touch Detected (%d)! Faking UP command.", touch_value);
            cmd = IR_CMD_UP;
        }

        static int loop_count = 0;
        loop_count++;

        if (cmd != IR_CMD_NONE) {
            int64_t uptime_ms = esp_timer_get_time() / 1000;
            int uptime_sec = (int)(uptime_ms / 1000);

            // Decode button name
            const char* button_name = "UNKNOWN";
            switch(cmd) {
                case IR_CMD_UP: button_name = "UP"; break;
                case IR_CMD_DOWN: button_name = "DOWN"; break;
                case IR_CMD_LEFT: button_name = "LEFT"; break;
                case IR_CMD_RIGHT: button_name = "RIGHT"; break;
                case IR_CMD_OK: button_name = "OK"; break;
                case IR_CMD_STAR: button_name = "* (STAR)"; break;
                case IR_CMD_HASH: button_name = "# (HASH)"; break;
                case IR_CMD_0: button_name = "0"; break;
                case IR_CMD_1: button_name = "1"; break;
                case IR_CMD_2: button_name = "2"; break;
                case IR_CMD_3: button_name = "3"; break;
                case IR_CMD_4: button_name = "4"; break;
                case IR_CMD_5: button_name = "5"; break;
                case IR_CMD_6: button_name = "6"; break;
                case IR_CMD_7: button_name = "7"; break;
                case IR_CMD_8: button_name = "8"; break;
                case IR_CMD_9: button_name = "9"; break;
                default: button_name = "UNKNOWN"; break;
            }

            ESP_LOGI(TAG, "");
            ESP_LOGI(TAG, "╔════════════════════════════════════════╗");
            ESP_LOGI(TAG, "║ BUTTON PRESS DETECTED                  ║");
            ESP_LOGI(TAG, "║ Elapsed Time: %5d.%03d sec            ║", uptime_sec, (int)(uptime_ms % 1000));
            ESP_LOGI(TAG, "║ Button: %-15s              ║", button_name);
            ESP_LOGI(TAG, "║ Code: 0x%08X                      ║", (unsigned int)cmd);
            ESP_LOGI(TAG, "╚════════════════════════════════════════╝");
        }

        if (loop_count % 50 == 0) {
            ESP_LOGI(TAG, "Ready... (loop %d)", loop_count);
        }

        acebott_read_line_sensors(&left_val, &mid_val, &right_val);

#if ENABLE_ULTRASONIC
        float dist = acebott_get_distance();
#else
        float dist __attribute__((unused)) = 0.0f;
#endif

        // --- SECTION 2: DIAGNOSTICS ---
        // Get time since boot in milliseconds (ms)
        int32_t uptime_ms __attribute__((unused)) = (int32_t)(esp_timer_get_time() / 1000);

        // Update the log string to include the literal words "Elapsed Time"
        /*ESP_LOGI(TAG, "Elapsed Time: %.1f s | STATS | Dist: %.1f | Line: %d-%d-%d | Speed: %d | Dir: %d",
                 uptime_ms / 1000.0, dist, left_val, mid_val, right_val, current_speed, (int)current_dir);*/

        // Removed gpio_set_level(2, 0) - was interfering with headlights!

        // --- SECTION 3: SAFETY VETO (Obstacle Avoidance) ---
        // If forward and too close to a wall, force a stop.
        bool obstacle_stop = false;
#if ENABLE_ULTRASONIC
        if (dist > 0 && dist < 15.0f && current_dir == MOTOR_FORWARD) {
            ESP_LOGI(TAG, "OBSTACLE DETECTED! Distance: %.1f cm - STOPPING", dist);
            acebott_move(MOTOR_STOP, 0);
            obstacle_stop = true;
        }
#endif

        // --- SECTION 4: USER INTENT (Remote Control) ---
        // Manual override from IR remote takes highest priority.
        if (cmd != IR_CMD_NONE) {
            // TEST MODE: Just report what button was pressed, don't move motors
            switch (cmd) {
                case IR_CMD_UP:
                    ESP_LOGI(TAG, "✓ UP button detected - would move FORWARD");
                    acebott_beep(2000, 100);
                    break;
                case IR_CMD_DOWN:
                    ESP_LOGI(TAG, "✓ DOWN button detected - would move BACKWARD");
                    acebott_beep(1800, 100);
                    break;
                case IR_CMD_LEFT:
                    ESP_LOGI(TAG, "✓ LEFT button detected - would turn CCW");
                    acebott_beep(1600, 100);
                    break;
                case IR_CMD_RIGHT:
                    ESP_LOGI(TAG, "✓ RIGHT button detected - would turn CW");
                    acebott_beep(2200, 100);
                    break;
                case IR_CMD_OK:
                    ESP_LOGI(TAG, "✓ OK button detected - would flash headlights");
                    acebott_flash_headlights(1, 100);
                    break;
                case IR_CMD_STAR:
                    ESP_LOGI(TAG, "✓ STAR (*) button detected - BEEP TEST!");
                    acebott_beep(2000, 500);  // Long beep
                    break;
                case IR_CMD_HASH:
                    ESP_LOGI(TAG, "✓ HASH (#) button detected - HEADLIGHT TEST!");
                    acebott_flash_headlights(3, 200);
                    break;
                default:
                    ESP_LOGI(TAG, "✗ UNKNOWN button: Code 0x%08X", (unsigned int)cmd);
                    break;
            }
        }

        // --- SECTION 5: NAVIGATION (Line Following) ---
        // Only active if we are already moving and the path is clear.
        else if (current_dir != MOTOR_STOP && !obstacle_stop) {
            int black_line_threshold = 2000;
            if (mid_val >= black_line_threshold) {
                ESP_LOGI(TAG, "Line Follow: Going STRAIGHT (mid sensor active)");
                acebott_move(current_dir, current_speed); // Straight
            } else if (left_val >= black_line_threshold) {
                ESP_LOGI(TAG, "Line Follow: Turning LEFT (left sensor active)");
                acebott_move(MOTOR_CCW, current_speed);   // Left
            } else if (right_val >= black_line_threshold) {
                ESP_LOGI(TAG, "Line Follow: Turning RIGHT (right sensor active)");
                acebott_move(MOTOR_CW, current_speed);    // Right
            }
        }

        // --- SECTION 6: LOOP TIMING ---
        vTaskDelay(pdMS_TO_TICKS(200)); // Completes the 50ms loop cycle
    }
}