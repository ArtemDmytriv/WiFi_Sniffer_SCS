#ifndef _OLED_DISP_H
#define _OLED_DISP_H

extern "C" {
#include "ssd1306.h"
#include "font8x8_basic.h"
}

void initialize_ssd1306(SSD1306_t *dev);

#endif