#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    MODE_IDLE = 0,
    MODE_LEFT,
    MODE_RIGHT,
    MODE_BOTH,
    MODE_BRAKE,
    MODE_WARNING,
    MODE_BURST,
    MODE_LINK_LOSS
} bike_mode_t;

typedef enum {
    INPUT_EVENT_NONE = 0,
    INPUT_EVENT_LEFT_TRIGGER,
    INPUT_EVENT_RIGHT_TRIGGER,
    INPUT_EVENT_UP_TRIGGER,
    INPUT_EVENT_DOWN_TRIGGER,
    INPUT_EVENT_CENTER_PRESS
} input_event_t;

typedef struct {
    float speed_mps;
    bike_mode_t rear_mode;
    uint32_t heartbeat_tick;
} telemetry_t;

#endif