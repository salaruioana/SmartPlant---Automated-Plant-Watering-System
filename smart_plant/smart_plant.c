#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "ssd1306.h"
#include "button.h"
#include "menu.h"
#include "sensor.h"
#include "servo.h"

#define I2C_PORT    i2c0
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5

// UART
#define UART_ID      uart0
#define UART_TX_PIN  0
#define UART_RX_PIN  1

int main() {
    stdio_init_all();
    while (!stdio_usb_connected()) sleep_ms(100);
    printf("Smart Plant starting...\n");

    // Init I2C + OLED
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    oled_init(I2C_PORT);

    // Init UART
    uart_init(UART_ID, 115200);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // Init module
    sensor_init();
    button_init();
    menu_init();
    servo_init();

    printf("Toate initializate!\n");

    uint64_t last_check_time = 0;   // ultima verificare umiditate
    uint64_t last_uart_time  = 0;   // ultima trimitere UART

    while (true) {
        uint64_t now = time_us_64();

        // 1. Citeste umiditate (la fiecare 2 secunde)
        static int umiditate = 0;
        if (now - last_check_time > 2000000) {
            if (watering_get_state() != WATERING_ACTIVE) { 
                umiditate = sensor_get_humidity();
            }
            last_check_time = now;
        }

        // 2. Verifica buton
        ButtonEvent ev = button_get_event();
        if (ev != BUTTON_NONE) {
            menu_handle_event(ev);
        }

        // 3. Logica udare (doar cand e profil activ)
        if (menu_get_state() == STATE_CONFIRMED) {
            const ProfilPlanta *p = menu_get_active_profile();
            WateringState ws = watering_get_state();

            if (ws == WATERING_IDLE && !watering_cooldown_active()&& umiditate < p->prag_uscat) {
                watering_start(p->durata_udare);
            }

            watering_update(); // actualizeaza starea si inchide servo daca e cazul

            // Dupa ce display-ul a avut un ciclu sa afiseze DONE, resetam la IDLE
            if (watering_get_state() == WATERING_DONE) {
                watering_reset();
            }

        }

        // 4. Update display
        static uint64_t last_display_time = 0;
        bool servo_activ = (watering_get_state() == WATERING_ACTIVE);

        // daca servo uda: refresh rar (2s) ca sa nu interfereze cu PWM
        // daca nu uda: refresh normal (500ms)
        uint64_t display_interval = servo_activ ? 2000000 : 500000;

        if (now - last_display_time > display_interval) {
            menu_draw(umiditate, watering_get_state(), watering_get_seconds_left());
            last_display_time = now;
        }

        // 5. Trimite date pe UART la fiecare 5 secunde
        if (now - last_uart_time > 5000000) {
            const ProfilPlanta *p = menu_get_active_profile();
            char buf[128];
            // Folosim umiditate si profil indiferent de starea udarii
            int a_udat = (watering_get_state() == WATERING_ACTIVE) ? 1 : 0;
            int sec = watering_get_seconds_left(); // 0 daca nu uda, normal altfel
            snprintf(buf, sizeof(buf),
                "{\"umiditate\":%d,\"profil\":\"%s\","
                "\"udare\":%d,\"sec_left\":%d}\n",
                umiditate,
                (p != NULL) ? p->nume : "niciunul",  // protectie null
                a_udat,
                sec);
            uart_puts(UART_ID, buf);
            printf("%s", buf);
            last_uart_time = now;
        }

    }
}