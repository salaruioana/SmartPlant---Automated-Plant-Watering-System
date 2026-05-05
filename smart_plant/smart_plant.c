

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"

#define I2C_PORT    i2c0
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5

int main() {
    stdio_init_all();
    while (!stdio_usb_connected()) sleep_ms(100);

    // Init I2C
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    // Init OLED
    oled_init(I2C_PORT);
    printf("OLED initializat!\n");

    // Test afisare
    oled_clear();
    oled_draw_text(0, 0,  "Smart Plant v1.0");
    oled_draw_text(0, 12, "Umiditate: 42%");
    oled_draw_progress_bar(0, 24, 128, 10, 42);
    oled_draw_text(0, 40, "Profil: Temperat");
    oled_draw_text(0, 52, "Status: Sol OK");
    oled_show();
    printf("Test OLED afisat!\n");

    while (true) sleep_ms(1000);
}
