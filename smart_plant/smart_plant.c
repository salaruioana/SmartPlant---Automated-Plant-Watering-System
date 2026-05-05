#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include "hardware/timer.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"

// I2C pentru OLED - i2c0 pe GP4/GP5
#define I2C_PORT     i2c0
#define I2C_SDA_PIN  4
#define I2C_SCL_PIN  5

// ADC pentru senzor umiditate
#define SENSOR_PIN   26   // ADC0

// PWM pentru servo
#define SERVO_PIN    15

// Buton
#define BUTTON_PIN   2

const char src[] = "Smart Plant - DMA OK!";
char dst[count_of(src)];

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    printf("Timer callback fired!\n");
    return 0;
}

int main() {
    // USB stdio - printf merge prin cablul USB la PC
    stdio_init_all();
    sleep_ms(2000); // asteapta sa se initializeze USB

    printf("=== Smart Plant - Test perifericele ===\n");

    // --- TEST I2C ---
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    printf("I2C initializat pe GP%d (SDA) si GP%d (SCL)\n", I2C_SDA_PIN, I2C_SCL_PIN);

    // Scanare I2C - cauta dispozitive conectate (OLED ar trebui sa apara la 0x3C)
    printf("Scanare I2C...\n");
    for (int addr = 0; addr < 128; addr++) {
        uint8_t buf;
        int ret = i2c_read_blocking(I2C_PORT, addr, &buf, 1, false);
        if (ret >= 0) {
            printf("  Dispozitiv gasit la adresa: 0x%02X\n", addr);
        }
    }

    // --- TEST ADC ---
    adc_init();
    adc_gpio_init(SENSOR_PIN);
    adc_select_input(0); // ADC0 = GP26
    uint16_t val_adc = adc_read();
    printf("ADC citire initiala: %d (din 4095)\n", val_adc);

    // --- TEST PWM (servo) ---
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(SERVO_PIN);
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, 64.0f);
    pwm_config_set_wrap(&cfg, 39062); // ~50Hz
    pwm_init(slice, &cfg, true);
    // pozitie 0 grade
    pwm_set_gpio_level(SERVO_PIN, 1000);
    printf("PWM initializat pe GP%d - servo la 0 grade\n", SERVO_PIN);

    // --- TEST DMA ---
    int chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, true);
    dma_channel_configure(chan, &c, dst, src, count_of(src), true);
    dma_channel_wait_for_finish_blocking(chan);
    printf("DMA test: '%s'\n", dst);

    // --- TEST TIMER ---
    add_alarm_in_ms(2000, alarm_callback, NULL, false);
    printf("Timer setat - callback in 2 secunde\n");

    // --- TEST BUTON ---
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
    printf("Buton initializat pe GP%d\n", BUTTON_PIN);
    printf("\nApasat butonul pentru a testa...\n");

    // --- BUCLA PRINCIPALA DE TEST ---
    while (true) {
        // citeste ADC
        uint16_t adc_val = adc_read();
        int umiditate = 100 - (adc_val * 100 / 4095); // brut, fara calibrare
        
        // citeste buton
        bool apasat = gpio_get(BUTTON_PIN); // pull-up: 0 = apasat

        printf("Umiditate bruta: %d%% | ADC: %d | Buton: %s\n",
               umiditate, adc_val, apasat ? "APASAT" : "liber");

        sleep_ms(500);
    }
}