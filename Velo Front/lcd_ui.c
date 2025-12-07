#include "lcd_ui.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "LCD";

#define LCD_HOST    SPI2_HOST
#define PIN_NUM_MISO -1
#define PIN_NUM_MOSI GPIO_NUM_10
#define PIN_NUM_CLK  GPIO_NUM_8
#define PIN_NUM_CS   GPIO_NUM_4
#define PIN_NUM_DC   GPIO_NUM_5
#define PIN_NUM_RST  GPIO_NUM_6
#define PIN_NUM_BCKL GPIO_NUM_7

#define COLOR_BLACK   0x0000
#define COLOR_RED     0xF800
#define COLOR_YELLOW  0xFFE0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F
#define COLOR_WHITE   0xFFFF

static spi_device_handle_t spi;

#define LCD_WIDTH  320
#define LCD_HEIGHT 240

void lcd_cmd(const uint8_t cmd) {
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &cmd;
    t.user = (void*)0; 
    gpio_set_level(PIN_NUM_DC, 0);
    ret = spi_device_polling_transmit(spi, &t);
    ESP_ERROR_CHECK(ret);
}

void lcd_data(const uint8_t *data, int len) {
    if (len == 0) return;
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = len * 8;
    t.tx_buffer = data;
    t.user = (void*)1; 
    gpio_set_level(PIN_NUM_DC, 1);
    ret = spi_device_polling_transmit(spi, &t);
    ESP_ERROR_CHECK(ret);
}

void lcd_fill_color(uint16_t color) {
    uint8_t data[4];
    
    lcd_cmd(0x2A);
    data[0] = 0; data[1] = 0;
    data[2] = (LCD_WIDTH-1) >> 8; data[3] = (LCD_WIDTH-1) & 0xFF;
    lcd_data(data, 4);

    lcd_cmd(0x2B);
    data[0] = 0; data[1] = 0;
    data[2] = (LCD_HEIGHT-1) >> 8; data[3] = (LCD_HEIGHT-1) & 0xFF;
    lcd_data(data, 4);

    lcd_cmd(0x2C); 
    
    const int lines = 20; 
    uint16_t *buffer = heap_caps_malloc(LCD_WIDTH * lines * 2, MALLOC_CAP_DMA);
    if (!buffer) return;

    for (int i = 0; i < LCD_WIDTH * lines; i++) {
        buffer[i] = (color >> 8) | (color << 8); 
    }

    int remaining = LCD_HEIGHT;
    while (remaining > 0) {
        int lines_to_send = (remaining > lines) ? lines : remaining;
        lcd_data((uint8_t*)buffer, lines_to_send * LCD_WIDTH * 2);
        remaining -= lines_to_send;
    }
    free(buffer);
}

void lcd_ui_init(void) {
    gpio_set_direction(PIN_NUM_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_BCKL, GPIO_MODE_OUTPUT);

    gpio_set_level(PIN_NUM_RST, 0); vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(PIN_NUM_RST, 1); vTaskDelay(pdMS_TO_TICKS(100));

    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO, 
        .mosi_io_num = PIN_NUM_MOSI, 
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1, 
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_WIDTH * LCD_HEIGHT * 2 + 100
    };
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 40*1000*1000, 
        .mode = 0, 
        .spics_io_num = PIN_NUM_CS, 
        .queue_size = 7,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(LCD_HOST, &devcfg, &spi));

    lcd_cmd(0x11); 
    vTaskDelay(pdMS_TO_TICKS(120));

    lcd_cmd(0x36); 
    uint8_t madctl = 0x70; 
    lcd_data(&madctl, 1);

    lcd_cmd(0x3A); 
    uint8_t fmt = 0x55; 
    lcd_data(&fmt, 1);

    lcd_cmd(0x21); 
    lcd_cmd(0x29); 
    
    gpio_set_level(PIN_NUM_BCKL, 1);
    lcd_fill_color(COLOR_BLACK);
}

static bike_mode_t last_mode = MODE_IDLE;

void lcd_ui_update(bike_mode_t mode, float speed_mps, bool link_ok) {
    if (mode != last_mode) {
        switch (mode) {
            case MODE_LEFT:   lcd_fill_color(COLOR_YELLOW); break;
            case MODE_RIGHT:  lcd_fill_color(COLOR_CYAN);   break;
            case MODE_WARNING:lcd_fill_color(COLOR_RED);    break;
            case MODE_BURST:  lcd_fill_color(COLOR_WHITE);  break;
            case MODE_IDLE:   lcd_fill_color(COLOR_BLACK);  break;
            case MODE_LINK_LOSS: lcd_fill_color(COLOR_MAGENTA); break;
            default: break;
        }
        last_mode = mode;
    }
}