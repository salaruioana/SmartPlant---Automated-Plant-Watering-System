#include "servo.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"
#include <stdio.h>
 // dacă e Pico

/*RP2040 (chip-ul de pe Raspberry Pi Pico) are:
8 PWM slices (numerotate 0–7)
Fiecare slice are 2 canale: A și B
Deci total: 16 canale PWM.
Orice pin GPIO mapat pe funcția PWM aparține automat unui slice și unui canal.*/
static uint pwm_slice;
static uint pwm_chan;


// stare udare
static WateringState watering_state = WATERING_IDLE;
static uint64_t watering_start_time = 0;
static uint32_t watering_duration_ms = 0;
static uint64_t last_watering_end_time = 0;


void servo_init(void) {
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    // folosim functiile de bibllioteca pentru a gasi canalul PWM asociat
    pwm_slice = pwm_gpio_to_slice_num(SERVO_PIN);
    pwm_chan  = pwm_gpio_to_channel(SERVO_PIN);


    // 50Hz: clkdiv=125, wrap=20000 → 1 tick = 1us
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, 125.0f);  // 125MHz / 125 = 1MHz → 1 tick = 1µs
    pwm_config_set_wrap(&cfg, 20000);
    pwm_init(pwm_slice, &cfg, true);


    servo_close();
    printf("Servo initializat pe GP%d\n", SERVO_PIN);
}


void servo_set_position(uint16_t pulse_us) {
    pwm_set_chan_level(pwm_slice, pwm_chan, pulse_us);
}


void servo_open(void) {
    servo_set_position(SERVO_OPEN_US);
    printf("Servo: DESCHIS\n");
}


void servo_close(void) {
    servo_set_position(SERVO_CLOSED_US);
    printf("Servo: INCHIS\n");
}


void watering_start(uint16_t durata_secunde) {
    if (watering_state == WATERING_ACTIVE) return; // deja uda


    watering_state     = WATERING_ACTIVE;
    watering_start_time = time_us_64();
    watering_duration_ms = durata_secunde * 1000;


    servo_open();
    printf("Udare pornita: %d secunde\n", durata_secunde);
}


// functie de actualizare a starii servomotorului
void watering_update(void) {
    if (watering_state != WATERING_ACTIVE) return;


    uint64_t elapsed_ms = (time_us_64() - watering_start_time) / 1000;


    if (elapsed_ms >= watering_duration_ms) {
       servo_close();
        watering_state = WATERING_DONE;
        printf("Udare terminata!\n");
    }
}


WateringState watering_get_state(void) {
    return watering_state;
}


// functie apelata de interfata pentru afisarea timpului de udare ramas
int watering_get_seconds_left(void) {
    if (watering_state != WATERING_ACTIVE) return 0;


    uint64_t elapsed_ms = (time_us_64() - watering_start_time) / 1000;
   
    // protectie underflow
    if (elapsed_ms >= watering_duration_ms) return 0;
   
    int left = (int)((watering_duration_ms - elapsed_ms) / 1000) + 1;
    return left > 0 ? left : 0;
}


// dupa un delay scurt, resetam starea de udare
void watering_reset(void) {
    last_watering_end_time = time_us_64();
    watering_state = WATERING_IDLE;
}


//pentru a evita multe udari consecutive neintrerupt - lasam apa sa intre in sol
bool watering_cooldown_active(void) {
    if (last_watering_end_time == 0) return false;
    uint64_t elapsed_ms = (time_us_64() - last_watering_end_time) / 1000;
    return elapsed_ms < MIN_INTERVAL_BETWEEN_WATERINGS_MS;
}
