#include "acebott_hw.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <driver/adc.h>

static int current_speed = 0;

void process_remote_command(ir_button_t btn) {
    switch(btn) {
        case IR_CMD_UP:
            current_speed = (current_speed + 50 > 255) ? 255 : current_speed + 50;
        acebott_beep(440 + current_speed, 100); // Rising pitch
        acebott_move(MOTOR_FORWARD, current_speed);
        break;

        case IR_CMD_DOWN:
            current_speed = (current_speed - 50 < 0) ? 0 : current_speed - 50;
        acebott_beep(880 - (255 - current_speed), 100); // Falling pitch
        acebott_move(MOTOR_FORWARD, current_speed);
        break;

        case IR_CMD_STOP:
            current_speed = 0;
        acebott_move(MOTOR_STOP, 0);
        acebott_beep(220, 500); // Low warning tone
        break;

        case IR_CMD_OK:
            // Trigger the "Train Station" sequence
                printf("Sequence Start!\n");
        break;

        default:
            break;
    }
}

void app_main(void) {
    acebott_init();

    while(1) {
        // 1. Safety Veto
        float distance = acebott_get_distance();

        if (distance < 15.0f && current_speed > 0) {
            // EMERGENCY STOP - Overrides everything else
            acebott_move(MOTOR_STOP, 0);
            acebott_beep(880, 500); // High-pitched warning beep
            current_speed = 0;      // Reset speed to zero
            printf("Safety Stop: Obstacle at %.2f cm\n", distance);
        }

        // 2. User Intent
        ir_button_t last_button = acebott_get_ir_command();
        if (last_button != IR_CMD_NONE) {
            process_remote_command(last_button);
        }

        // --- Section 3: The Road Thingy Logic ---
        int left_val, mid_val, right_val;
        int black_line_threshold = 2000;

        // CALL the function here, don't DEFINE it here!
        acebott_read_line_sensors(&left_val, &mid_val, &right_val);

        if (left_val < black_line_threshold && mid_val >= black_line_threshold && right_val < black_line_threshold) {
            acebott_move(MOTOR_FORWARD, current_speed);
        }
        // ... (rest of your nudge logic)

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
