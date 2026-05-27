#include "menu.h"
#include "ssd1306.h"
#include "button.h"
#include <stdio.h>

// vector cu meniurile posibile - se observa usurinta de extindere a aplicatiei
const ProfilPlanta profiluri[NUM_PROFILES] = {
    {"Arid",        "cactus, suculente",   15,  2,  1440},
    {"Mediteran","lavanda, rozmarin",   30,  4,   720},
    {"Temperat",    "monstera, pothos",    45,  6,   480},
    {"Tropical",    "orhidee, ferigi",     60,  8,   240},
    {"Acvatic",     "nufar, papirus",      80, 15,    60},
};

// indexul folosit in functionalitatea de navigare printre meniuri
static int current_index = 0;
// indexul folosit pentru a marca starea stabila activa
static int active_index  = 0;
// meniul poate fi in starea de navigare prin optiuni sau poate fi programat intr-un mod
static AppState state = STATE_MENU;

void menu_init(void) {
    current_index = 0;
    active_index  = 0;
    state = STATE_MENU;
}

// logica de navigare printre profiluri in functie de starea butonului
void menu_handle_event(ButtonEvent ev) {
    if (state == STATE_MENU) {
        // apasarea scurta in timpul navigarii inseamna trecerea la urmatorul profil
        if (ev == BUTTON_SHORT) {
            current_index = (current_index + 1) % NUM_PROFILES;
        // a expirat durata de stabilizare si se poate intra in modul stabil in profilul curent
        } else if (ev == BUTTON_AUTO_SELECT) {
            active_index = current_index;
            state = STATE_CONFIRMED;
            watering_reset(); //reseteaza udarea cand se confirma un profil nou
        }
    } else if (state == STATE_CONFIRMED) {
        // daca este un profil in desfasurare si se apasa butonul, se doreste trecerea la un alt profil
        if (ev == BUTTON_SHORT) {
            state = STATE_MENU;
            watering_reset(); ////  reseteaza udarea când se revine în meniu
        }
    }
}

// realizeaza afisarea datelor 
// primeste ca parametri datele de la senzor, starea (udare in desfasurare sau nu + cat mai dureaza udarea)
void menu_draw(int umiditate_procent, WateringState watering, int sec_left) {
    // se curata display-ul de datele vechi
    oled_clear();

    //navigare prin profiluri
    if (state == STATE_MENU) {
        const ProfilPlanta *p = &profiluri[current_index];
        oled_draw_text(0, 0, "Selecteaza profil:");
        oled_draw_text_large(0, 12, p->nume);
        oled_draw_text(0, 30, p->exemple);

        char buf[32];
        //snprintf function in C is used to write formatted output to a sized buffer.
        snprintf(buf, sizeof(buf), "Prag:%d%%  Udare:%ds",p->prag_uscat, p->durata_udare);
        oled_draw_text(0, 42, buf);

        // actualizeaza pe meniu de cat timp se afla in starea respectiva
        uint64_t elapsed_ms = (time_us_64() - button_get_last_press_time()) / 1000;
        int auto_sec = 0;
        if (elapsed_ms < AUTO_SELECT_MS) {
            auto_sec = (int)((AUTO_SELECT_MS - elapsed_ms) / 1000) + 1;
        }
        char countdown[32];
        snprintf(countdown, sizeof(countdown), ">>next  auto:%ds", auto_sec);
        oled_draw_text(0, 54, countdown);

    } else {
        // profil stabil
        const ProfilPlanta *p = &profiluri[active_index];
        // monitorizare udare
        if (watering == WATERING_ACTIVE) {
            // 
            oled_draw_text(0, 0, "Se uda...");

            // timer mare vizibil
            char timer[16];
            snprintf(timer, sizeof(timer), "%ds", sec_left);
            oled_draw_text_large(40, 16, timer);

            // bara progres udare
            int total = p->durata_udare;
            int percent = (total - sec_left) * 100 / total;
            oled_draw_progress_bar(0, 40, 128, 10, percent);

            oled_draw_text(0, 54, p->nume);

        } else {
            // monitorizare normala
            oled_draw_text(0, 0, "Profil activ:");
            oled_draw_text_large(0, 10, p->nume);

            char buf[32];
            snprintf(buf, sizeof(buf), "Umiditate: %d%%", umiditate_procent);
            oled_draw_text(0, 30, buf);

            oled_draw_progress_bar(0, 40, 128, 10, umiditate_procent);

            // status udare
            if (watering == WATERING_DONE) {
                oled_draw_text(0, 54, "Udat recent");
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