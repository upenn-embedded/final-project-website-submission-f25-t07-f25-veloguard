#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "common.h"

void state_machine_init(void);
void state_machine_on_input(input_event_t evt);
void state_machine_on_link_loss(bool lost);
bike_mode_t state_machine_get_mode(void);

#endif