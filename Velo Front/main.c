#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "common.h"
#include "input_joystick.h"
#include "state_machine.h"
#include "ble_client.h"
#include "lcd_ui.h"

static const char *TAG = "MAIN";

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    input_init();
    state_machine_init();
    lcd_ui_init();
    ble_client_init();

    telemetry_t rear_telemetry;
    bike_mode_t current_mode;
    input_event_t input_evt;
    bool link_status;

    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(50); 

    while (1) {
        if (input_get_event(&input_evt)) {
            state_machine_on_input(input_evt);
        }

        link_status = ble_client_is_link_ok();
        state_machine_on_link_loss(!link_status);

        current_mode = state_machine_get_mode();
        ble_client_get_telemetry(&rear_telemetry);

        ble_client_set_desired_mode(current_mode);
        lcd_ui_update(current_mode, rear_telemetry.speed_mps, link_status);

        vTaskDelayUntil(&last_wake_time, frequency);
    }
}