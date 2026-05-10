#ifndef SENSOR_H
#define SENSOR_H

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include <stdint.h>

#define SENSOR_PIN       26
#define SENSOR_ADC_CH    0
#define NUM_SAMPLES      32  // medie mobila pe 32 citiri

void sensor_init(void);
int  sensor_get_humidity(void);   // returneaza 0-100%
int  sensor_get_raw(void);        // returneaza valoare bruta ADC

// Calibrare - ruleaza o data in aer si o data in apa
void sensor_calibrate_dry(void);  // seteaza valoarea pentru sol uscat
void sensor_calibrate_wet(void);  // seteaza valoarea pentru sol ud

#endif