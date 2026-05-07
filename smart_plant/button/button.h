#ifndef BUTTON_H
#define BUTTON_H

#include "pico/stdlib.h"
#include <stdbool.h>

#define BUTTON_PIN      2
#define DEBOUNCE_MS     30    // ignora semnale sub 50ms
#define AUTO_SELECT_MS  10000 // 10 secunde fara apasare = selectare automata

typedef enum {
    BUTTON_NONE,
    BUTTON_SHORT,
    BUTTON_AUTO_SELECT  // timeout 10 secunde
} ButtonEvent;

void button_init(void);
ButtonEvent button_get_event(void);
uint64_t button_get_last_press_time(void);

#endif