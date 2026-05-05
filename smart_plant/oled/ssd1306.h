
#ifndef SSD1306_H
#define SSD1306_H

#include "hardware/i2c.h"
#include <stdint.h>
#include <stdbool.h>

// Dimensiuni display
#define OLED_WIDTH   128
#define OLED_HEIGHT  64
#define OLED_ADDR    0x3C

// Initializare
void oled_init(i2c_inst_t *i2c);

// Operatii de baza
void oled_clear(void);
void oled_show(void);

// Desenare
void oled_draw_pixel(int x, int y, bool on);
void oled_draw_text(int x, int y, const char *text);
void oled_draw_text_large(int x, int y, const char *text);
void oled_draw_rect(int x, int y, int w, int h, bool on);
void oled_draw_filled_rect(int x, int y, int w, int h, bool on);
void oled_draw_progress_bar(int x, int y, int w, int h, int percent);

#endif

