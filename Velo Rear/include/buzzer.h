#ifndef BUZZER_H
#define BUZZER_H

#include "common.h"

void buzzer_init(void);
// mode: current brake mode; dist_m: distance in meters from ultrasonic sensor
void buzzer_update(rear_brake_mode_t mode, float dist_m);

#endif
