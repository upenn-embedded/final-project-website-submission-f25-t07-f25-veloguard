#ifndef BLE_CLIENT_H
#define BLE_CLIENT_H

#include "common.h"

void ble_client_init(void);
void ble_client_set_desired_mode(bike_mode_t mode);
bool ble_client_get_telemetry(telemetry_t *t);
bool ble_client_is_link_ok(void);

#endif