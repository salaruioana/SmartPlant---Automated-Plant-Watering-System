#include "sensor.h"
#include <stdio.h>

// Buffer DMA - NUM_SAMPLES citiri ADC
static uint16_t adc_buffer[NUM_SAMPLES];
static int dma_chan = -1;

// Valori calibrare (default rezonabil pentru senzorul capacitiv)
// In aer = ~3200, in apa = ~800 
static uint16_t cal_dry = 3200;
static uint16_t cal_wet = 800;


/*
ADC produce valori → FIFO → DMA le mută → buffer
                     ↑
                  DREQ (semnal)
*/

void sensor_init(void) {
    // Init ADC
    adc_init();
    adc_gpio_init(SENSOR_PIN);
    adc_select_input(SENSOR_ADC_CH);

    // Activeaza modul FIFO pentru DMA
    // DMA = Direct Memory Access: Un modul hardware intern care poate transfera date între periferice și memorie fără a folosi procesorul (CPU).
    // DMA preia automat seturi de citiri din ADC FIFO și le pune în adc_buffer[], fără să mai fie nevoie ca CPU-ul să citească fiecare valoare.
    adc_fifo_setup(
        true,   // enable FIFO
        true,   // enable DMA request
        1,      // threshold - DMA request la fiecare sample
        false,  // no error bit
        false   // full 12 biti (nu 8)
    );

    // Viteza ADC: 48MHz / 96 = 500kHz
    // ADC_freq = clk_adc / clkdiv
    // clk_adc - ceasul hardware intern care alimentează convertorul analog-digital
    // clkdiv — divizorul de frecvență
    adc_set_clkdiv(96.0f);

    // Claim DMA channel
    dma_chan = dma_claim_unused_channel(true);

    printf("Sensor initializat pe GP%d, DMA canal %d\n", 
           SENSOR_PIN, dma_chan);
}

// Declanseaza o citire DMA de NUM_SAMPLES sample-uri
static void sensor_trigger_dma(void) {
    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);
    
    // ADC-ul Pico produce 12 biți dar sunt stocați în 16 biți pentru simplitate
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    // Sursa NU se modifică (ADC FIFO)- DMA citește mereu din aceeași adresă
    channel_config_set_read_increment(&cfg, false);  // ADC FIFO = adresa fixa
    // Destinația se incrementează (buffer)
    channel_config_set_write_increment(&cfg, true);  // buffer = incrementam
    // DREQ = DMA Request
    channel_config_set_dreq(&cfg, DREQ_ADC);         // DMA request de la ADC

    // începe să copiezi ADC în buffer
    dma_channel_configure(
        dma_chan,
        &cfg,
        adc_buffer,          // destinatie
        &adc_hw->fifo,       // sursa = ADC FIFO
        NUM_SAMPLES,         // numar transferuri
        true                 // start imediat
    );

    // Porneste ADC in mod free-running
    adc_run(true);

    // CPU așteaptă finalizarea DMA: Asta blochează doar funcția, nu întreg sistemul.
    dma_channel_wait_for_finish_blocking(dma_chan);

    // Opreste ADC
    adc_run(false);
    adc_fifo_drain();
}

// Filtru medie mobila pe buffer-ul DMA
static uint16_t sensor_average(void) {
    uint32_t sum = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        sum += adc_buffer[i];
    }
    return (uint16_t)(sum / NUM_SAMPLES);
}

int sensor_get_raw(void) {
    sensor_trigger_dma();
    return sensor_average();
}

int sensor_get_humidity(void) {
    int raw = sensor_get_raw();

    // Mapeaza raw la procente folosind valorile de calibrare
    // cal_dry = valoare cand e uscat (procent mic)
    // cal_wet = valoare cand e ud (procent mare)
    // Senzorul capacitiv: uscat = valoare mare, ud = valoare mica
    
    if (raw >= cal_dry) return 0;
    if (raw <= cal_wet) return 100;

    int umiditate = (cal_dry - raw) * 100 / (cal_dry - cal_wet);
    return umiditate;
}

void sensor_calibrate_dry(void) {
    sensor_trigger_dma();
    cal_dry = sensor_average();
    printf("Calibrare uscat: %d\n", cal_dry);
}

void sensor_calibrate_wet(void) {
    sensor_trigger_dma();
    cal_wet = sensor_average();
    printf("Calibrare ud: %d\n", cal_wet);
}