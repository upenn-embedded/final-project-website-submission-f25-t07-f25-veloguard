#ifndef INPUT_JOYSTICK_H
#define INPUT_JOYSTICK_H

#include "common.h"

void input_init(void);
bool input_get_event(input_event_t *evt);

#endif