#include "button.h"
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include <stdint.h>
#include <stdbool.h>

static volatile uint64_t press_start    = 0;
static volatile uint64_t last_press_time = 0;
static volatile bool     pressed        = false;
static volatile ButtonEvent pending_event = BUTTON_NONE;

static void button_isr(uint gpio, uint32_t events) {
    uint64_t now = time_us_64();

    if (events & GPIO_IRQ_EDGE_FALL) {
        // buton apasat
        press_start = now;
        pressed = true;
    }

    if (events & GPIO_IRQ_EDGE_RISE && pressed) {
        // buton eliberat
        uint64_t duration_ms = (now - press_start) / 1000;
        pressed = false;

        if (duration_ms < DEBOUNCE_MS) {
            // zgomot mecanic, ignoram
            return;
        }

        // apasare valida
        last_press_time = now;
        pending_event = BUTTON_SHORT;
    }
}



void button_init(void) {
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    last_press_time = time_us_64(); // initializeaza timerul

    gpio_set_irq_enabled_with_callback(
        BUTTON_PIN,
        GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
        true,
        &button_isr
    );
}

uint64_t button_get_last_press_time(void) {
    return last_press_time;
}

ButtonEvent button_get_event(void) {
    uint64_t now = time_us_64();

    // Verifica timeout 10 secunde
    if (last_press_time > 0) {
        uint64_t elapsed_ms = (now - last_press_time) / 1000;
        if (elapsed_ms >= AUTO_SELECT_MS) {
            last_press_time = 0; // resetam ca sa nu mai declanseze
            return BUTTON_AUTO_SELECT;
        }
    }

    // Returneaza eveniment pending de la intrerupere
    ButtonEvent ev = pending_event;
    pending_event = BUTTON_NONE;
    return ev;
}