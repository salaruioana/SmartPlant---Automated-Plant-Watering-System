#ifndef MENU_H
#define MENU_H

#include "pico/stdlib.h"
#include "button.h"
#include <stdint.h>
#include "servo.h"

#define NUM_PROFILES 5

typedef struct {
    const char* nume;
    const char* exemple;
    uint8_t     prag_uscat;
    uint16_t    durata_udare;
    uint32_t    interval_min;
} ProfilPlanta;

typedef enum {
    STATE_MENU,
    STATE_CONFIRMED,
} AppState;

extern const ProfilPlanta profiluri[NUM_PROFILES];

void menu_init(void);
void menu_handle_event(ButtonEvent ev);
void menu_draw(int umiditate_procent, WateringState watering, int sec_left);
const ProfilPlanta* menu_get_active_profile(void);
AppState menu_get_state(void);

#endif