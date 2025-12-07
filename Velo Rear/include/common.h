#ifndef COMMON_H
#define COMMON_H

#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    TURN_NONE = 0,
    TURN_LEFT,
    TURN_RIGHT,
    TURN_HAZARD
} rear_turn_mode_t;

typedef enum {
    BRAKE_IDLE = 0,
    BRAKE_STEADY,
    BRAKE_WARNING,
    BRAKE_BURST,
    BRAKE_LINK_LOSS
} rear_brake_mode_t;

typedef struct {
    rear_turn_mode_t turn;
    rear_brake_mode_t brake;
} rear_state_t;

#endif
