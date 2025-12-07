#ifndef LCD_UI_H
#define LCD_UI_H

#include "common.h"

void lcd_ui_init(void);
void lcd_ui_update(bike_mode_t mode, float speed_mps, bool link_ok);

#endif