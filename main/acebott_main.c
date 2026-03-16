/* TIMESTAMP: [03/16/26 14:48] */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "acebott_hw.h"

static const char *TAG = "ACEBOTT_BRAIN";

void app_main(void) {
    // 1. Initialize Hardware
    acebott_init();
    ESP_LOGI(TAG, "Acebott System Online. Line Tracking & IR Ready.");

    // 2. Initial Test Routine (Beeps & Lights)
    ESP_LOGI(TAG, "GPIO test round 1 - 3 beeps BEFORE");
    acebott_beep(2000, 100);
    vTaskDelay(pdMS_TO_TICKS(100));
    acebott_beep(2000, 100);
    vTaskDelay(pdMS_TO_TICKS(100));
    acebott_beep(2000, 100);

    ESP_LOGI(TAG, "   >>> Headlights going HIGH <<<");
    acebott_set_headlights(true);
    vTaskDelay(pdMS_TO_TICKS(3000));

    ESP_LOGI(TAG, "   >>> Headlights going LOW <<<");
    acebott_set_headlights(false);
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "GPIO test round 1 - 3 beeps AFTER");
    acebott_beep(2000, 100);
    vTaskDelay(pdMS_TO_TICKS(100));
    acebott_beep(2000, 100);
    vTaskDelay(pdMS_TO_TICKS(100));
    acebott_beep(2000, 100);

    // 3. Brief Motor Test
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "Testing motors (direction=163, speed=150)...");
    acebott_move(MOTOR_FORWARD, 150);
    vTaskDelay(pdMS_TO_TICKS(500));
    acebott_move(MOTOR_STOP, 0);
    ESP_LOGI(TAG, "Motors stopped");

    ESP_LOGI(TAG, "ACEBOTT QD001 startup complete! Waiting for IR commands...");

    // 4. Main Control Loop
    while (1) {
        float dist = acebott_get_distance();
        ir_button_t cmd = acebott_get_ir_command();

        ESP_LOGI(TAG, "Sensor Check: %.1f cm | Dir: %d", dist, (int)cmd);

        if (cmd != IR_CMD_NONE) {
            ESP_LOGI(TAG, "IR Command Received: 0x%08X", (unsigned int)cmd);
            // Future command logic goes here
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}