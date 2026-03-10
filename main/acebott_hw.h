#ifndef ACEBOTT_HW_H
#define ACEBOTT_HW_H

#include <stdint.h>
#include <stdbool.h>

// 1. ENUMS
typedef enum {
    IR_CMD_UP      = 0xB946FF00,
    IR_CMD_DOWN    = 0xEA15FF00,
    IR_CMD_LEFT    = 0xBB44FF00,
    IR_CMD_RIGHT   = 0xBC43FF00,
    IR_CMD_OK      = 0xEB14FF00,
    IR_CMD_STOP    = 0xF609FF00,
    IR_CMD_AUTO    = 0xE31CFF00,
    IR_CMD_NONE    = 0x00000000
} ir_button_t;

typedef enum {
    MOTOR_STOP     = 0,
    MOTOR_FORWARD  = 163,
    MOTOR_BACK     = 92,
    MOTOR_CW       = 172,
    MOTOR_CCW      = 83
} motor_dir_t;

// 2. PUBLIC API
void acebott_init(void);
void acebott_move(motor_dir_t dir, uint8_t speed);
void acebott_beep(uint32_t freq, uint32_t duration_ms);
float acebott_get_distance(void);
ir_button_t acebott_get_ir_command(void);
void acebott_read_line_sensors(int *l, int *m, int *r);

// 3. UNIT TESTABLE MATH
float distance_cm_from_us(int64_t duration_us);

#endif // ACEBOTT_HW_H