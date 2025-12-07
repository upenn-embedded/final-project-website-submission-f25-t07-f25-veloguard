#include "input_joystick.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

static const char *TAG = "JOYSTICK";

#define ADC_CHAN_X          ADC_CHANNEL_2 
#define ADC_CHAN_Y          ADC_CHANNEL_3
#define ADC_CENTER_VAL      2100   
#define ADC_DEADZONE        900   

static adc_oneshot_unit_handle_t adc1_handle;
static bool is_axis_x_centered = true; 
static bool is_axis_y_centered = true; 

void input_init(void) {
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHAN_X, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHAN_Y, &config));
}

bool input_get_event(input_event_t *evt) {
    *evt = INPUT_EVENT_NONE;
    int val_x = 0;
    int val_y = 0;

    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHAN_X, &val_x));
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHAN_Y, &val_y));

    bool left = (val_x < (ADC_CENTER_VAL - ADC_DEADZONE));
    bool right = (val_x > (ADC_CENTER_VAL + ADC_DEADZONE));
    
    if (!left && !right) {
        is_axis_x_centered = true;
    } else if (is_axis_x_centered) {
        if (left) {
            *evt = INPUT_EVENT_LEFT_TRIGGER;
        } else if (right) {
            *evt = INPUT_EVENT_RIGHT_TRIGGER;
        }
        is_axis_x_centered = false;
        return true;
    }

    bool up = (val_y < (ADC_CENTER_VAL - ADC_DEADZONE));
    bool down = (val_y > (ADC_CENTER_VAL + ADC_DEADZONE));

    if (!up && !down) {
        is_axis_y_centered = true;
    } else if (is_axis_y_centered) {
        if (up) {
            *evt = INPUT_EVENT_UP_TRIGGER;
        } else if (down) {
            *evt = INPUT_EVENT_DOWN_TRIGGER;
        }
        is_axis_y_centered = false;
        return true;
    }

    return false;
}