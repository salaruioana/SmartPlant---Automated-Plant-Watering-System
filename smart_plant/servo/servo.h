#ifndef SERVO_H
#define SERVO_H


#include "pico/stdlib.h"
#include <stdbool.h>
#include <stdint.h>


#define SERVO_PIN        15
/*Pentru SG90:
500 µs ≈ 0°
1500 µs ≈ 90°
2400–2500 µs ≈ 180°
*/
#define SERVO_CLOSED_US  800   // 0 grade = blocat
#define SERVO_OPEN_US    2500  // 90 grade = deschis
#define MIN_INTERVAL_BETWEEN_WATERINGS_MS  30000  // 30 secunde minim intre udari


typedef enum {
    WATERING_IDLE,      // nu uda
    WATERING_ACTIVE,    // uda acum
    WATERING_DONE,      // tocmai a terminat
} WateringState;


void servo_init(void);
void servo_set_position(uint16_t pulse_us);
void servo_open(void);
void servo_close(void);


// Logica udare non-blocanta
void watering_start(uint16_t durata_secunde);
void watering_update(void);           // apelat in bucla principala
WateringState watering_get_state(void);
int watering_get_seconds_left(void);  // secunde ramase
void watering_reset(void);
bool watering_cooldown_active(uint32_t interval_ms);


#endif
