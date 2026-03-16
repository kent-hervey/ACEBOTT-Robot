/* TIMESTAMP: [03/16/26 14:48] */
#ifndef ACEBOTT_HW_H
#define ACEBOTT_HW_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// --- PIN DEFINITIONS ---
#define PIN_MOTOR_EN        16
#define PIN_MOTOR_DATA      5
#define PIN_MOTOR_LATCH     17
#define PIN_MOTOR_CLOCK     18
#define PIN_PWM1            19
#define PIN_PWM2            23

#define PIN_ULTRASONIC_TRIG  13
#define PIN_ULTRASONIC_ECHO  14
#define PIN_LINE_L           35
#define PIN_LINE_M           36
#define PIN_LINE_R           39
#define IR_RECEIVER_PIN      4

#define SERVO_PIN            25
#define HEADLIGHT_L_GPIO     12
#define HEADLIGHT_R_GPIO     2
#define BUZZER_GPIO          33

#ifdef __cplusplus
extern "C" {
#endif

    /* --- IR REMOTE CODES --- */
    typedef enum {
        IR_CMD_UP      = 0x00FF629D,
        IR_CMD_DOWN    = 0x00FFA857,
        IR_CMD_LEFT    = 0x00FF22DD,
        IR_CMD_RIGHT   = 0x00FFC23D,
        IR_CMD_OK      = 0x00FF02FD,
        IR_CMD_STAR    = 0x00FF42BD,
        IR_CMD_HASH    = 0x00FF52AD,
        IR_CMD_NONE    = 0xFFFFFFFF
    } ir_button_t;

    /* --- MOTOR DIRECTIONS --- */
    typedef enum {
        MOTOR_STOP    = 0,
        MOTOR_FORWARD = 163,
        MOTOR_BACK    = 92,
        MOTOR_CW      = 172,
        MOTOR_CCW     = 83
    } motor_dir_t;

    /* --- FUNCTION PROTOTYPES --- */
    void acebott_init(void);
    void acebott_move(motor_dir_t dir, uint8_t speed);
    void acebott_beep(uint32_t freq, uint32_t duration_ms);
    void acebott_set_headlights(bool on);
    void acebott_flash_headlights(int count, uint32_t duration_ms);
    void acebott_read_line_sensors(uint32_t *l, uint32_t *m, uint32_t *r);
    float acebott_get_distance(void);
    ir_button_t acebott_get_ir_command(void);

#ifdef __cplusplus
}
#endif

#endif