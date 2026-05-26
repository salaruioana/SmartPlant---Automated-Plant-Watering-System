#pragma once
#include <string.h>
#include "pico/sync.h"
#include "servo.h"
#define ASSERT(cond, msg) \
    do { if (!(cond)) { printf("EROARE: %s (linia %d)\n", msg, __LINE__); panic(msg); } } while(0)


// implementează comunicarea sigură între Core 0 și Core 1.
// Este un Shared State (memorie comună) protejat cu spinlock.
// spinlock = „lacăt hardware” între core-uri
typedef struct {
    int umiditate;
    WateringState watering;
    int sec_left;
    char profil_nume[16];
} SharedState;

extern SharedState g_shared;
extern spin_lock_t *g_lock;

static inline void shared_write(int um, WateringState ws, int sl, const char *pn) {
    uint32_t save = spin_lock_blocking(g_lock);
    g_shared.umiditate = um;
    g_shared.watering  = ws;
    g_shared.sec_left  = sl;
    strncpy(g_shared.profil_nume, pn ? pn : "niciunul", 15);
    g_shared.profil_nume[15] = '\0';
    spin_unlock(g_lock, save);
}

static inline SharedState shared_read(void) {
    uint32_t save = spin_lock_blocking(g_lock);
    SharedState copy = g_shared;
    spin_unlock(g_lock, save);
    return copy;
}