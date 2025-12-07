#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "common.h"

void state_machine_init(void);
void state_machine_update(float dist, rear_turn_mode_t turn, bool link_ok, bool decel);
rear_state_t state_machine_get(void);

#endif
