#include "ble_client.h"
#include "esp_log.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BLE";

static bool link_connected = false;
static bike_mode_t mode_to_send = MODE_IDLE;
static telemetry_t latest_telemetry = { .speed_mps = 0.0f, .rear_mode = MODE_IDLE, .heartbeat_tick = 0 };
static uint32_t last_heartbeat_time = 0;

void ble_sim_task(void *pvParams) {
    while (1) {
        link_connected = true;
        static float speed = 0.0;
        static int dir = 1;
        
        speed += (0.5 * dir);
        if (speed > 10.0) dir = -1;
        if (speed < 0.0) dir = 1;

        latest_telemetry.speed_mps = speed;
        latest_telemetry.heartbeat_tick++;
        last_heartbeat_time = xTaskGetTickCount();
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void ble_client_init(void) {
    xTaskCreate(ble_sim_task, "ble_sim", 2048, NULL, 5, NULL);
}

void ble_client_set_desired_mode(bike_mode_t mode) {
    mode_to_send = mode;
}

bool ble_client_get_telemetry(telemetry_t *t) {
    if (t) {
        memcpy(t, &latest_telemetry, sizeof(telemetry_t));
        return true;
    }
    return false;
}

bool ble_client_is_link_ok(void) {
    if ((xTaskGetTickCount() - last_heartbeat_time) > pdMS_TO_TICKS(3000)) {
        link_connected = false;
    }
    return link_connected;
}