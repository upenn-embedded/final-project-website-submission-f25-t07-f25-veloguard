#include "state_machine.h"
#include "esp_log.h"

static const char *TAG = "FSM";

static bike_mode_t current_mode = MODE_IDLE;
static bike_mode_t previous_mode = MODE_IDLE;
static bool is_link_lost = false;

void state_machine_init(void) {
    current_mode = MODE_IDLE;
}

void state_machine_on_input(input_event_t evt) {
    if (is_link_lost) return; 

    switch (evt) {
        case INPUT_EVENT_LEFT_TRIGGER:
            if (current_mode == MODE_LEFT) current_mode = MODE_IDLE; 
            else current_mode = MODE_LEFT;
            break;

        case INPUT_EVENT_RIGHT_TRIGGER:
            if (current_mode == MODE_RIGHT) current_mode = MODE_IDLE;
            else current_mode = MODE_RIGHT;
            break;

        case INPUT_EVENT_DOWN_TRIGGER:
            if (current_mode == MODE_WARNING) current_mode = MODE_IDLE;
            else current_mode = MODE_WARNING;
            break;

        case INPUT_EVENT_UP_TRIGGER:
            if (current_mode == MODE_BURST) current_mode = MODE_IDLE;
            else current_mode = MODE_BURST;
            break;

        default:
            break;
    }
}

void state_machine_on_link_loss(bool lost) {
    if (lost && !is_link_lost) {
        previous_mode = current_mode;
        current_mode = MODE_LINK_LOSS;
        is_link_lost = true;
    } else if (!lost && is_link_lost) {
        current_mode = previous_mode;
        is_link_lost = false;
    }
}

bike_mode_t state_machine_get_mode(void) {
    return current_mode;
}