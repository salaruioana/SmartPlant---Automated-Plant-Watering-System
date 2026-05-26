#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "hardware/sync.h"
#include "ssd1306.h"
#include "button.h"
#include "menu.h"
#include "sensor.h"
#include "servo.h"
#include "shared.h"

#define I2C_PORT    i2c0
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5

#define UART_ID     uart0
#define UART_TX_PIN 0
#define UART_RX_PIN 1

// Definitii globale pentru shared state (declarate extern in shared.h)
SharedState g_shared = {0};
spin_lock_t *g_lock = NULL;

// CORE 1 — doar display OLED
// se actualizează la 0.5 secunde
void core1_entry(void) {
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    oled_init(I2C_PORT);

    while (true) {
        SharedState s = shared_read();
        menu_draw(s.umiditate, s.watering, s.sec_left);
        sleep_ms(500);
    }
}

// CORE 0 — logica principala
int main(void) {
    stdio_init_all();
    sleep_ms(3000);
    printf("Smart Plant starting...\n");

    // Spinlock inainte de lansarea Core 1
    g_lock = spin_lock_init(spin_lock_claim_unused(true));

    // Lanseaza Core 1 — I2C si OLED se init acolo
    multicore_launch_core1(core1_entry);

    // Init UART
    uart_init(UART_ID, 115200);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // Init module Core 0
    sensor_init();
    button_init(); 
    menu_init();
    sleep_ms(1000);
    servo_init();

    printf("Toate initializate!\n");

    menu_handle_event(BUTTON_AUTO_SELECT);
    
    uint64_t last_check_time = 0;
    uint64_t last_uart_time  = 0;
    static int umiditate = 0;

    while (true) {
        uint64_t now = time_us_64();

        // 1. Citeste umiditate la fiecare 2 secunde
        if (now - last_check_time > 2000000) {
            if (watering_get_state() != WATERING_ACTIVE) {
                umiditate = sensor_get_humidity();
            }
            last_check_time = now;
        }

        // 2. Buton — decommenteaza cand ai buton nou
        ButtonEvent ev = button_get_event();
        if (ev != BUTTON_NONE) {
            menu_handle_event(ev);
        }

        // 3. Logica udare
        if (menu_get_state() == STATE_CONFIRMED) {
            const ProfilPlanta *p = menu_get_active_profile();
            WateringState ws = watering_get_state();

            if (ws == WATERING_IDLE && !watering_cooldown_active() && umiditate < p->prag_uscat) {
                watering_start(p->durata_udare);
            }

            watering_update();

            if (watering_get_state() == WATERING_DONE) {
                watering_reset();
            }
        }

        // 4. Actualizeaza shared_state pentru Core 1
        {
            const ProfilPlanta *p = menu_get_active_profile();
            shared_write(
                umiditate,
                watering_get_state(),
                watering_get_seconds_left(),
                p ? p->nume : "niciunul"
            );
        }

        // 5. UART la fiecare 5 secunde
        if (now - last_uart_time > 5000000) {
            const ProfilPlanta *p = menu_get_active_profile();
            char buf[128];
            int a_udat = (watering_get_state() == WATERING_ACTIVE) ? 1 : 0;
            int sec    = watering_get_seconds_left();
            snprintf(buf, sizeof(buf),
                "{\"umiditate\":%d,\"profil\":\"%s\","
                "\"udare\":%d,\"sec_left\":%d}\n",
                umiditate,
                p ? p->nume : "niciunul",
                a_udat,
                sec);
            uart_puts(UART_ID, buf);
            printf("%s", buf);
            last_uart_time = now;
        }

        tight_loop_contents();
    }
}