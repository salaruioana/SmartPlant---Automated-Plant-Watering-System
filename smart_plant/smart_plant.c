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
    sleep_ms(3000);
    printf("Smart Plant starting...\n");

    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    oled_init(I2C_PORT);

    uart_init(UART_ID, 115200);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    sensor_init();
    // button_init();  <-- scos
    menu_init();
    sleep_ms(1000);
    servo_init();

    printf("Toate initializate!\n");

    // Selecteaza automat primul profil fara buton
    menu_handle_event(BUTTON_AUTO_SELECT);

    uint64_t last_check_time = 0;
    uint64_t last_uart_time  = 0;

    while (true) {
        uint64_t now = time_us_64();

        static int umiditate = 0;
        if (now - last_check_time > 2000000) {
            if (watering_get_state() != WATERING_ACTIVE) {
                umiditate = sensor_get_humidity();
            }
            last_check_time = now;
        }

        // buton scos complet

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

        static uint64_t last_display_time = 0;
        bool servo_activ = (watering_get_state() == WATERING_ACTIVE);
        uint64_t display_interval = servo_activ ? 2000000 : 500000;

        if (now - last_display_time > display_interval) {
            menu_draw(umiditate, watering_get_state(), watering_get_seconds_left());
            last_display_time = now;
        }

        if (now - last_uart_time > 5000000) {
            const ProfilPlanta *p = menu_get_active_profile();
            char buf[128];
            int a_udat = (watering_get_state() == WATERING_ACTIVE) ? 1 : 0;
            int sec = watering_get_seconds_left();
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
    }
}