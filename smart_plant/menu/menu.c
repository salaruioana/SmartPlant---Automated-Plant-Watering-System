#include "menu.h"
#include "ssd1306.h"
#include "button.h"
#include <stdio.h>

const ProfilPlanta profiluri[NUM_PROFILES] = {
    {"Arid",        "cactus, suculente",   15,  2,  1440},
    {"Mediteran","lavanda, rozmarin",   30,  4,   720},
    {"Temperat",    "monstera, pothos",    45,  6,   480},
    {"Tropical",    "orhidee, ferigi",     60,  8,   240},
    {"Acvatic",     "nufar, papirus",      80, 15,    60},
};

static int current_index = 0;
static int active_index  = 0;
static AppState state    = STATE_MENU;

void menu_init(void) {
    current_index = 0;
    active_index  = 0;
    state = STATE_MENU;
}

void menu_handle_event(ButtonEvent ev) {
    if (state == STATE_MENU) {
        if (ev == BUTTON_SHORT) {
            current_index = (current_index + 1) % NUM_PROFILES;
        } else if (ev == BUTTON_AUTO_SELECT) {
            active_index = current_index;
            state = STATE_CONFIRMED;
        }
    } else if (state == STATE_CONFIRMED) {
        if (ev == BUTTON_SHORT) {
            state = STATE_MENU;
        }
    }
}

void menu_draw(int umiditate_procent, WateringState watering, int sec_left) {
    oled_clear();

    if (state == STATE_MENU) {
        const ProfilPlanta *p = &profiluri[current_index];
        oled_draw_text(0, 0, "Selecteaza profil:");
        oled_draw_text_large(0, 12, p->nume);
        oled_draw_text(0, 30, p->exemple);

        char buf[32];
        snprintf(buf, sizeof(buf), "Prag:%d%%  Udare:%ds",
                 p->prag_uscat, p->durata_udare);
        oled_draw_text(0, 42, buf);

        uint64_t elapsed_ms = (time_us_64() - button_get_last_press_time()) / 1000;
        int auto_sec = (int)((AUTO_SELECT_MS - elapsed_ms) / 1000) + 1;
        if (auto_sec < 0) auto_sec = 0;
        char countdown[32];
        snprintf(countdown, sizeof(countdown), ">>next  auto:%ds", auto_sec);
        oled_draw_text(0, 54, countdown);

    } else {
        // ===== ECRAN ACTIV =====
        const ProfilPlanta *p = &profiluri[active_index];

        if (watering == WATERING_ACTIVE) {
            // ===== UDARE IN CURS =====
            oled_draw_text(0, 0, "Se uda...");

            // Timer mare vizibil
            char timer[16];
            snprintf(timer, sizeof(timer), "%ds", sec_left);
            oled_draw_text_large(40, 16, timer);

            // Bara progres udare
            int total = p->durata_udare;
            int percent = (total - sec_left) * 100 / total;
            oled_draw_progress_bar(0, 40, 128, 10, percent);

            oled_draw_text(0, 54, p->nume);

        } else {
            // ===== MONITORING NORMAL =====
            oled_draw_text(0, 0, "Profil activ:");
            oled_draw_text_large(0, 10, p->nume);

            char buf[32];
            snprintf(buf, sizeof(buf), "Umiditate: %d%%", umiditate_procent);
            oled_draw_text(0, 30, buf);

            oled_draw_progress_bar(0, 40, 128, 10, umiditate_procent);

            // Status cu iconita
            if (watering == WATERING_DONE) {
                oled_draw_text(0, 54, "Udat recent :)");
            } else if (umiditate_procent < p->prag_uscat) {
                oled_draw_text(0, 54, "Sol uscat! Udare...");
            } else {
                oled_draw_text(0, 54, "Sol OK");
            }
        }
    }

    oled_show();
}

const ProfilPlanta* menu_get_active_profile(void) {
    return &profiluri[active_index];
}

AppState menu_get_state(void) {
    return state;
}