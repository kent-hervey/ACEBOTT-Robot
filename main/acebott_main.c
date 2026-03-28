/* ================================================================
   ACEBOTT QD001 - Main Control Loop
   Branch: experiment-control
   Last Updated: 2026-03-28

   IR Remote: NEC protocol, receiver on GPIO 4
   All 17 buttons confirmed working.

   TOGGLES:
     USE_ULTRASONIC  0 = ignore sensor, 1 = auto-stop at 15cm
     USE_MOTORS      0 = log only (safe bench test), 1 = real movement
   ================================================================ */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "acebott_hw.h"
#include <inttypes.h>

#define USE_ULTRASONIC  1   // Set to 1 when ultrasonic sensor is plugged in
#define USE_MOTORS      1   // Set to 0 for bench testing (logs only, no movement)

static const char *TAG = "ACEBOTT";

// Speed constants - tuned for Mecanum wheels
#define SPEED_STEP          50   // Speed change per button press
#define MIN_MOTOR_SPEED    150   // Jump-start speed to overcome gear friction
#define MAX_MOTOR_SPEED    255
#define STEER_NUDGE_MS     150   // How long a LEFT/RIGHT turn lasts (ms)
#define STEER_NUDGE_SPEED  160   // Speed during a turn

void app_main(void) {

    // ── 1. HARDWARE INIT ──────────────────────────────────────────
    acebott_init();
    ESP_LOGI(TAG, "ACEBOTT QD001 Online. Motors=%d Ultrasonic=%d",
             USE_MOTORS, USE_ULTRASONIC);

    // ── 2. STARTUP SELF-TEST ──────────────────────────────────────
    acebott_beep(2000, 100);
    vTaskDelay(pdMS_TO_TICKS(100));
    acebott_beep(2500, 100);
    vTaskDelay(pdMS_TO_TICKS(100));
    acebott_beep(3000, 100);          // ascending triple beep = ready

    acebott_set_headlights(true);
    vTaskDelay(pdMS_TO_TICKS(500));
    acebott_set_headlights(false);
    vTaskDelay(pdMS_TO_TICKS(200));
    acebott_set_headlights(true);     // double flash = online

    // Brief motor test so you can confirm wiring before unplugging USB
#if USE_MOTORS
    vTaskDelay(pdMS_TO_TICKS(500));
    acebott_move(MOTOR_FORWARD, MIN_MOTOR_SPEED);
    vTaskDelay(pdMS_TO_TICKS(300));
    acebott_move(MOTOR_STOP, 0);
    ESP_LOGI(TAG, "Motor test complete. Waiting for IR...");
#else
    ESP_LOGI(TAG, "USE_MOTORS=0 — motor test skipped. Waiting for IR...");
#endif

    // ── 3. STATE ──────────────────────────────────────────────────
    int current_speed = 0;
    motor_dir_t current_dir = MOTOR_STOP;
    bool stopped_by_obstacle = false;

    // ── 4. MAIN LOOP ──────────────────────────────────────────────
    while (1) {

        // ── 4a. ULTRASONIC PROXIMITY & AUTO-STOP ─────────────────
#if USE_ULTRASONIC
        float dist = acebott_get_distance();

        if (dist > 0 && current_dir == MOTOR_FORWARD) {
            if (dist < 15.0f) {
                // AUTO-STOP
                current_speed = 0;
                current_dir = MOTOR_STOP;
                acebott_move(MOTOR_STOP, 0);
                stopped_by_obstacle = true;
                ESP_LOGW(TAG, "AUTO-STOP: %.1f cm", dist);
                acebott_beep(600, 400);
            } else if (dist < 30.0f) {
                // APPROACHING WARNING — beep gets faster as you get closer
                // dist 30cm = 300ms gap, dist 15cm = 0ms gap
                uint32_t gap_ms = (uint32_t)((dist - 15.0f) * 20.0f);
                acebott_beep(1800, 50);
                if (gap_ms > 0) vTaskDelay(pdMS_TO_TICKS(gap_ms));
                ESP_LOGI(TAG, "PROXIMITY: %.1f cm", dist);
            }
        }

        // AUTO-RESUME when obstacle clears
        if (stopped_by_obstacle && dist > 25.0f) {
            stopped_by_obstacle = false;
            current_speed = 150;
            current_dir = MOTOR_FORWARD;
            acebott_move(MOTOR_FORWARD, 150);
            ESP_LOGI(TAG, "OBSTACLE CLEARED: %.1f cm — resuming at 150", dist);
            acebott_beep(2000, 80);
            vTaskDelay(pdMS_TO_TICKS(60));
            acebott_beep(2500, 80);
        }
#endif

        // ── 4b. IR REMOTE ─────────────────────────────────────────
        ir_button_t cmd = acebott_get_ir_command();
        if (cmd == IR_CMD_NONE) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        ESP_LOGI(TAG, "IR: 0x%08X", (unsigned int)cmd);

        switch (cmd) {

            case IR_CMD_UP:
                if (current_dir == MOTOR_STOP || current_dir == MOTOR_FORWARD) {
                    current_dir = MOTOR_FORWARD;
                    if (current_speed == 0)
                        current_speed = MIN_MOTOR_SPEED;       // torque jump
                    else
                        current_speed = (current_speed + SPEED_STEP > MAX_MOTOR_SPEED)
                                        ? MAX_MOTOR_SPEED : current_speed + SPEED_STEP;
                    ESP_LOGI(TAG, "UP → FORWARD speed=%d", current_speed);
                } else {
                    // Was going backward — decelerate
                    current_speed -= SPEED_STEP;
                    if (current_speed < MIN_MOTOR_SPEED) {
                        current_speed = 0;
                        current_dir = MOTOR_STOP;
                        ESP_LOGI(TAG, "UP → STOPPED (was reversing)");
                    } else {
                        ESP_LOGI(TAG, "UP → slowing reverse, speed=%d", current_speed);
                    }
                }
#if USE_MOTORS
                acebott_move(current_dir, current_speed);
#endif
                acebott_beep(2500, 50);
                break;

            case IR_CMD_DOWN:
                if (current_dir == MOTOR_STOP || current_dir == MOTOR_BACK) {
                    current_dir = MOTOR_BACK;
                    if (current_speed == 0)
                        current_speed = MIN_MOTOR_SPEED;       // torque jump
                    else
                        current_speed = (current_speed + SPEED_STEP > MAX_MOTOR_SPEED)
                                        ? MAX_MOTOR_SPEED : current_speed + SPEED_STEP;
                    ESP_LOGI(TAG, "DOWN → REVERSE speed=%d", current_speed);
                } else {
                    // Was going forward — decelerate
                    current_speed -= SPEED_STEP;
                    if (current_speed < MIN_MOTOR_SPEED) {
                        current_speed = 0;
                        current_dir = MOTOR_STOP;
                        ESP_LOGI(TAG, "DOWN → STOPPED (was forward)");
                    } else {
                        ESP_LOGI(TAG, "DOWN → slowing forward, speed=%d", current_speed);
                    }
                }
#if USE_MOTORS
                acebott_move(current_dir, current_speed);
#endif
                acebott_beep(1500, 50);
                break;

            case IR_CMD_LEFT:
                ESP_LOGI(TAG, "LEFT → CCW nudge %dms", STEER_NUDGE_MS);
#if USE_MOTORS
                acebott_move(MOTOR_CCW, STEER_NUDGE_SPEED);
                vTaskDelay(pdMS_TO_TICKS(STEER_NUDGE_MS));
                acebott_move(current_dir, current_speed);  // resume
#endif
                acebott_beep(1800, 50);
                break;

            case IR_CMD_RIGHT:
                ESP_LOGI(TAG, "RIGHT → CW nudge %dms", STEER_NUDGE_MS);
#if USE_MOTORS
                acebott_move(MOTOR_CW, STEER_NUDGE_SPEED);
                vTaskDelay(pdMS_TO_TICKS(STEER_NUDGE_MS));
                acebott_move(current_dir, current_speed);  // resume
#endif
                acebott_beep(2200, 50);
                break;

            case IR_CMD_OK:
                current_speed = 0;
                current_dir = MOTOR_STOP;
#if USE_MOTORS
                acebott_move(MOTOR_STOP, 0);
#endif
                ESP_LOGI(TAG, "OK → EMERGENCY STOP");
                acebott_flash_headlights(2, 100);
                acebott_beep(1000, 300);
                break;

            case IR_CMD_STAR:
                ESP_LOGI(TAG, "STAR → beep test");
                acebott_beep(2000, 100);
                vTaskDelay(pdMS_TO_TICKS(100));
                acebott_beep(2000, 100);
                break;

            case IR_CMD_HASH:
                ESP_LOGI(TAG, "HASH → headlight flash test");
                acebott_flash_headlights(3, 150);
                break;

            // ── Number buttons — speed presets ───────────────────
            // 1-3 = slow, 4-6 = medium, 7-9 = fast, 0 = stop
            default: {
                // Map raw codes to speed presets
                uint32_t raw = (uint32_t)cmd;
                int preset = -1;
                if      (raw == 0x00FF6897) preset = 1;
                else if (raw == 0x00FF9867) preset = 2;
                else if (raw == 0x00FFB04F) preset = 3;
                else if (raw == 0x00FF30CF) preset = 4;
                else if (raw == 0x00FF18E7) preset = 5;
                else if (raw == 0x00FF7A85) preset = 6;
                else if (raw == 0x00FF10EF) preset = 7;
                else if (raw == 0x00FF38C7) preset = 8;
                else if (raw == 0x00FF5AA5) preset = 9;
                else if (raw == 0x00FF4AB5) preset = 0;

                if (preset == 0) {
                    current_speed = 0;
                    current_dir = MOTOR_STOP;
#if USE_MOTORS
                    acebott_move(MOTOR_STOP, 0);
#endif
                    ESP_LOGI(TAG, "0 → STOP");
                    acebott_beep(800, 200);
                } else if (preset > 0) {
                    // 1=50, 2=75, 3=100, 4=130, 5=155, 6=180, 7=200, 8=228, 9=255
                    const int speed_map[] = {0,50,75,100,130,155,180,200,228,255};
                    current_speed = speed_map[preset];
                    if (current_speed < MIN_MOTOR_SPEED)
                        current_speed = MIN_MOTOR_SPEED;  // never below stall
                    if (current_dir == MOTOR_STOP)
                        current_dir = MOTOR_FORWARD;      // default to forward
#if USE_MOTORS
                    acebott_move(current_dir, current_speed);
#endif
                    ESP_LOGI(TAG, "%d → speed preset %d", preset, current_speed);
                    acebott_beep(1500 + (preset * 100), 50);
                } else {
                    ESP_LOGW(TAG, "Truly unknown code: 0x%08" PRIx32, raw);
                }
                break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}