/* TIMESTAMP: [03/16/26 15:58] - RESTORING CONDITIONAL ULTRASONIC BLOCK */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "acebott_hw.h"

static const char *TAG = "ACEBOTT_BRAIN";

// Change this to 1 to enable Ultrasonic, 0 to block it out like this morning
#define USE_ULTRASONIC 1

static int current_speed = 0;
static motor_dir_t current_dir = MOTOR_STOP;

void app_main(void) {
    acebott_init();
    ESP_LOGI(TAG, "Acebott System Online. Manual Control Mode.");

    while (1) {

#if USE_ULTRASONIC
        float dist = acebott_get_distance();
        if (dist > 0 && dist < 15.0 && current_dir == MOTOR_FORWARD) {
            ESP_LOGW(TAG, "AUTO-STOP: Object at %.1f cm", dist);
            current_speed = 0;
            current_dir = MOTOR_STOP;
            acebott_move(MOTOR_STOP, 0);
        }
#endif

        ir_button_t cmd = acebott_get_ir_command();
        if (cmd != IR_CMD_NONE) {
            // Logging the command hex so you can see if DOWN is hitting
            ESP_LOGI(TAG, "IR Command: 0x%08X", (unsigned int)cmd);

            if (cmd == IR_CMD_UP) {
                current_dir = MOTOR_FORWARD;
                current_speed += 50;
                if (current_speed > 255) current_speed = 255;
                acebott_move(current_dir, current_speed);
            }
            else if (cmd == IR_CMD_DOWN) {
                // If moving forward, slow down. If stopped/back, move backward.
                if (current_dir == MOTOR_FORWARD && current_speed > 0) {
                    current_speed -= 50;
                    if (current_speed <= 0) {
                        current_speed = 0;
                        current_dir = MOTOR_STOP;
                    }
                } else {
                    current_dir = MOTOR_BACK;
                    current_speed = 150;
                }
                acebott_move(current_dir, current_speed);
            }
            else if (cmd == IR_CMD_LEFT) {
                acebott_move(MOTOR_CCW, 160);
                vTaskDelay(pdMS_TO_TICKS(150));
                acebott_move(current_dir, current_speed);
            }
            else if (cmd == IR_CMD_RIGHT) {
                acebott_move(MOTOR_CW, 160);
                vTaskDelay(pdMS_TO_TICKS(150));
                acebott_move(current_dir, current_speed);
            }
            else if (cmd == IR_CMD_OK) {
                current_speed = 0;
                current_dir = MOTOR_STOP;
                acebott_move(MOTOR_STOP, 0);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}