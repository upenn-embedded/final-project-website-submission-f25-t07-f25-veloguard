#ifndef BRAKE_LED_H
#define BRAKE_LED_H

#include "common.h"

void brake_led_init(void);
void brake_led_update(rear_brake_mode_t mode);

#endif
