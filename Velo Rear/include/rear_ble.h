#ifndef REAR_BLE_H
#define REAR_BLE_H

#include "common.h"
#include <stdbool.h>

void rear_ble_init(uint32_t baud);
void rear_ble_poll(void);
rear_turn_mode_t rear_ble_get_turn_cmd(void);
bool rear_ble_is_link_ok(void);

#endif
