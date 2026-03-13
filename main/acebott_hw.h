#ifndef ACEBOTT_HW_H
#define ACEBOTT_HW_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Pins */
#define IR_RECEIVER_PIN 4   // ACEBOTT official wiring (was 34 for noise testing)
#define SERVO_PIN 25
#define HEADLIGHT_L_GPIO 12
#define HEADLIGHT_R_GPIO 2
#define BUZZER_GPIO 33

#ifdef __cplusplus
extern "C" {
#endif

    /* 1. ENUMS */
    // ACTUAL codes from user's QD001 remote (confirmed via testing)
    typedef enum {
        IR_CMD_UP      = 0x00FF629D,  // UP arrow (confirmed)
        IR_CMD_DOWN    = 0x00FFA857,  // DOWN arrow (confirmed)
        IR_CMD_LEFT    = 0x00FF22DD,  // LEFT arrow (confirmed)
        IR_CMD_RIGHT   = 0x00FFC23D,  // RIGHT arrow (confirmed)
        IR_CMD_OK      = 0x00FF02FD,  // OK/Enter button (confirmed)
        IR_CMD_STAR    = 0x00FF42BD,  // * key (confirmed)
        IR_CMD_HASH    = 0x00FF52AD,  // # key (confirmed)
        IR_CMD_0       = 0x00FF4AB5,  // Number 0 (confirmed)
        IR_CMD_1       = 0x00FF6897,  // Number 1 (confirmed)
        IR_CMD_2       = 0x00FF9867,  // Number 2 (confirmed)
        IR_CMD_3       = 0x00FFB04F,  // Number 3 (confirmed)
        IR_CMD_4       = 0x00FF30CF,  // Number 4 (confirmed)
        IR_CMD_5       = 0x00FF18E7,  // Number 5 (confirmed)
        IR_CMD_6       = 0x00FF7A85,  // Number 6 (confirmed)
        IR_CMD_7       = 0x00FF10EF,  // Number 7 (confirmed)
        IR_CMD_8       = 0x00FF38C7,  // Number 8 (confirmed)
        IR_CMD_9       = 0x00FF5AA5,  // Number 9 (confirmed)
        IR_CMD_NONE    = 0xFFFFFFFF
    } ir_button_t;

    typedef enum {
        MOTOR_STOP     = 0,
        MOTOR_FORWARD  = 163,
        MOTOR_BACK     = 92,
        MOTOR_CW       = 172,
        MOTOR_CCW      = 83
    } motor_dir_t;

    /* 2. PUBLIC API */
    void acebott_init(void);
    void acebott_move(motor_dir_t dir, uint8_t speed);
    void acebott_beep(uint32_t freq, uint32_t duration_ms);
    void acebott_set_headlights(bool on);
    void acebott_flash_headlights(int count, uint32_t duration_ms);
    float acebott_get_distance(void);
    ir_button_t acebott_get_ir_command(void);
    void acebott_read_line_sensors(int *l, int *m, int *r);

    /* 3. UNIT TESTABLE MATH */
    float distance_cm_from_us(int64_t duration_us);

#ifdef __cplusplus
}
#endif

#endif // ACEBOTT_HW_H