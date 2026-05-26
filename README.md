# SmartPlant: Sistem de monitorizare și irigare 

**Echipa BerryBusy:** Panaite Elena-Alexandra și Șalaru Ioana (Grupa 1309B)

---

## Descrierea Proiectului
SmartPlant este un sistem automatizat, bazat pe Raspberry Pi Pico 2w, creat pentru a monitoriza starea de sănătate a plantelor de apartament. Dispozitivul măsoară umiditatea solului și decide dacă este necesară irigarea. Interfața oferă feedback vizual printr-un display OLED, iar execuția irigării este simulată mecanic folosind un servomotor.

### Obiectiv general 
Proiectarea și implementarea unui sistem embedded automat pentru monitorizarea și irigarea plantelor de apartament, utilizând Raspberry Pi Pico 2W și Pico SDK în C.

## Utilitate 
Proiectul **SmartPlant** rezolvă problema îngrijirii plantelor în lipsa utilizatorului (ex: în timpul concediilor) și optimizează consumul de resurse prin:
* **Automatizare completă:** Elimină eroarea umană (uitarea udării sau udarea excesivă) prin declanșarea pompei doar atunci când senzorul raportează un deficit de umiditate în sol.
* **Eficiență hidrică:** Previne risipa de apă și putrezirea rădăcinilor plantelor prin aplicarea unor cantități controlate de apă, adaptate strict necesarului setat în cod.
* **Monitorizare continuă:** Sistemul rulează non-stop (grație consumului redus al Raspberry Pi Pico 2W), asigurând un microclimat stabil pentru plantă.
---

## Cerințe Funcționale
Sistemul implementează următoarele funcționalități de bază:
* Achiziție de date (ADC): Citirea valorilor analogice de la senzorul capacitiv de umiditate a solului la intervale regulate.
* Interfață Utilizator (I2C): Afișarea stării curente a solului și a meniului de configurare pe un ecran OLED.
* Control Utilizator (GPIO / Întreruperi): Navigarea prin meniul de moduri de funcționare.
* Element de Execuție (PWM): Activarea unui servomotor SG90 la atingerea pragului de uscăciune, simulând deschiderea unei valve de apă.
* Monitorizare la distanță (UART): Transmiterea log-urilor și a stării sistemului către un PC prin interfața serială USB.

---

## Cerințe Non-funcționale
* Modularitate Software: Codul este dezvoltat în **C (Pico SDK)** și este modularizat (drivere separate sensor, servo, menu, button, ssd1306).
* Eficiență și Timing: Arhitectură non-blocantă bazată pe timestamp-uri (time_us_64()) — niciun modul nu folosește sleep() în bucla principală.
* Fiabilitate:  Filtru de medie mobilă pe 32 de citiri ADC prin DMA pentru eliminarea zgomotului electric al senzorului capacitiv.
* Paralelism: Utilizarea ambelor core-uri ARM Cortex-M33 ale RP2350 — Core 0 gestionează logica timp-real, Core 1 gestionează exclusiv display-ul OLED.

---

## Scenarii de Utilizare

**1. Configurarea inițială și selecția profilului**
* La pornirea dispozitivului, display-ul OLED afișează meniul de selecție profil cu un countdown de 10 secunde. Utilizatorul navighează prin cele 5 profile prin apăsări scurte ale butonului. După 10 secunde fără interacțiune, profilul curent este confirmat automat și sistemul începe monitorizarea.

**2. Declanșarea irigării automate**
* Sistemul citește umiditatea la fiecare 2 secunde. Când valoarea scade sub pragul profilului activ, servomotorul se rotește la 90° pentru durata configurată, simulând deschiderea unui robinet. Display-ul afișează un timer vizibil cu bara de progres. Simultan, pe serial se trimite un mesaj JSON cu "udare":1. După terminarea udării, sistemul intră în cooldown de 30 de secunde.

## Arhitectura proiectului
![Schema arhitecturii](docs/Arhitectura_proiect.jpeg)

## Arhitectura Software

**Structura modulară**
```
smart_plant/
├── smart_plant.c       — main(), Core 0, Core 1, shared state
├── button/
│   ├── button.h
│   └── button.c        — detecție apăsare prin întrerupere GPIO
├── menu/
│   ├── menu.h
│   └── menu.c          — mașină de stări, profile, desenare OLED
├── sensor/
│   ├── sensor.h
│   └── sensor.c        — citire ADC cu DMA, calibrare, medie mobilă
├── servo/
│   ├── servo.h
│   └── servo.c         — control PWM, logică udare non-blocantă
│
├──shared.h             — utilizarea paralelă a celor două procesoare
│
└── oled/
    ├── ssd1306.h
    └── ssd1306.c       — driver I2C, framebuffer, font 5x7
```

## Arhitectura dual-core

Sistemul utilizează ambele core-uri ale RP2350 pentru a elimina interferența dintre PWM (servo) și I2C (OLED):

### Core 0 — timp real, critic
- Citire senzor ADC prin DMA la fiecare 2 secunde  
- Logică udare: verificare prag, cooldown, acționare servo  
- Procesare evenimente buton prin întrerupere GPIO  
- Transmisie date JSON prin UART/USB la fiecare 5 secunde  
- Actualizare `shared_state` protejată cu spinlock  

### Core 1 — display
- Refresh OLED la interval fix de 500 ms  
- Citește `shared_state` în siguranță prin spinlock  
- I2C inițializat și utilizat exclusiv pe Core 1  

---

### Logica principală (Core 0)
Bucla `while(true)` din Core 0 este non-blocantă — nu conține niciun `sleep()`.  
Fiecare sarcină este executată doar dacă a trecut intervalul corespunzător, verificat prin `time_us_64()`:

| Sarcină                     | Interval                   |
|-----------------------------|----------------------------|
| Citire senzor umiditate     | 2 secunde                  |
| Verificare buton            | fiecare iterație (~µs)     |
| Logică udare                | fiecare iterație           |
| Actualizare `shared_state`  | fiecare iterație           |
| Transmisie UART/USB         | 5 secunde                  |


### Modulul sensor și logica de măsurare a umidității
Senzorul capacitiv produce o tensiune analogică invers proporțională cu umiditatea — sol uscat înseamnă tensiune mare, sol ud înseamnă tensiune mică. Citirea se face prin DMA pe 32 de sample-uri consecutive, după care se calculează media:
```c
umiditate% = (cal_dry - raw) * 100 / (cal_dry - cal_wet)
```
Valorile de calibrare implicite sunt cal_dry = 3200 (senzor în aer) și cal_wet = 800 (senzor în apă). Se recomandă calibrarea cu valorile reale măsurate.

### Modulul servo și logica de udare

Servomotorul SG90 simulează un robinet prin rotație între 0° (închis, puls 800µs) și 90° (deschis, puls 2500µs) la 50Hz. Logica de udare este non-blocantă — durata udării se numără prin timestamp, nu prin sleep().
Între două udări consecutive există un cooldown obligatoriu de 30 de secunde pentru a permite apei să fie absorbită de sol înainte de o nouă decizie.

### Modulul menu și profilele de udare

Sistemul include 5 profile predefinite pentru tipuri diferite de plante:

| Profil     | Exemple                 | Prag declanșare | Durată udare | Interval verificare |
|------------|--------------------------|------------------|--------------|----------------------|
| Arid       | cactus, suculente        | < 15%            | 2s           | 24h                  |
| Mediteran  | lavandă, rozmarin        | < 30%            | 4s           | 12h                  |
| Temperat   | monstera, pothos         | < 45%            | 6s           | 8h                   |
| Tropical   | orhidee, ferigi          | < 60%            | 8s           | 4h                   |
| Acvatic    | nufăr, papirus           | < 80%            | 15s          | 1h                   |

Navigarea între profile se face prin apăsare scurtă a butonului. După 10 secunde fără interacțiune, profilul curent este selectat automat (BUTTON_AUTO_SELECT). O apăsare scurtă în timpul monitorizării active revine în meniu.

### Modulul button și logica de navigare

Detecția apăsării se face prin întrerupere hardware pe GPIO_IRQ_EDGE_FALL și GPIO_IRQ_EDGE_RISE. Durata apăsării este măsurată cu time_us_64(). Semnalele sub 30ms sunt ignorate (debounce software). Evenimentele sunt stocate atomic în pending_event și consumate de bucla principală.

### Modulul ssd1306 și logica de afișare pe display

Driver complet pentru display-ul OLED 128×64px prin I2C. Folosește un framebuffer de 1024 bytes în RAM și un font 5×7 pixeli pentru caractere ASCII. Suportă text normal, text 2× mărit și bară de progres. Întregul framebuffer este trimis la display printr-un singur transfer I2C la fiecare refresh.

### Modulul `shared.h` — comunicare între Core-uri

Modulul `shared.h` implementează mecanismul de comunicare între Core 0 și Core 1 printr-un **Shared State** protejat cu **spinlock hardware**. Acesta permite schimb sigur de date între logica de control (Core 0) și interfața OLED (Core 1), fără race conditions.

### SharedState — memorie comună

```c
typedef struct {
    int umiditate;
    WateringState watering;
    int sec_left;
    char profil_nume[16];
} SharedState;
```

Accesul la `g_shared` este protejat cu un spinlock:

```c
extern SharedState g_shared;
extern spin_lock_t *g_lock;
```

Spinlock-ul asigură acces atomic între core-uri: un singur core poate citi/scrie la un moment dat, celălalt core așteaptă eliberarea lock-ului.

#### Sincronizare

CORE 0  ── shared_write() ──► [ g_shared + spinlock ] ◄── shared_read ── CORE 1

### Monitorizare la distanță (UART/USB)

Pico 2W transmite date prin USB serial (CDC) la fiecare 5 secunde în format JSON.  
Pentru monitorizare rapidă în timpul dezvoltării, datele au fost urmărite direct în **VS Code**, folosind extensia *Serial Monitor*, unde mesajele JSON apar în consolă în timp real:

```json
{"umiditate":45,"profil":"Temperat","udare":0,"sec_left":0}
```
![Mesaje primite prin interfata seriala](docs/consola.png)

## Tratarea Erorilor

Sistemul implementează un mecanism simplu și robust de tratare a erorilor, adaptat pentru un dispozitiv embedded.

Definit în `shared.h`, macro-ul `ASSERT` oprește sistemul curat atunci când o condiție critică nu este îndeplinită:

```c
#define ASSERT(cond, msg) \
    do { if (!(cond)) { printf("EROARE: %s (linia %d)\n", msg, __LINE__); panic(msg); } } while(0)
```

La declanșare, afișează mesajul de eroare și numărul liniei prin UART/USB, apoi apelează panic() din Pico SDK, care oprește execuția și face LED-ul onboard să clipească rapid, un semnal vizibil că sistemul a întâmpinat o problemă critică.

Este folosit exclusiv pentru erori nerecuperabile în faza de inițializare:
```c
g_lock = spin_lock_init(spin_lock_claim_unused(true));
ASSERT(g_lock != NULL, "Spinlock init failed");

ASSERT(sensor_init(), "Sensor init failed");
```

La inițializarea senzorului, canalul DMA este revendicat cu o verificare explicită; dacă hardware-ul nu are canale disponibile, sistemul oprește imediat execuția cu un mesaj clar:
```c
dma_chan = dma_claim_unused_channel(false);
ASSERT(dma_chan >= 0, "DMA channel unavailable");
```
Valorile ADC extreme de 0 sau 4095 sunt imposibile în condiții normale și indică un senzor deconectat sau defect. În acest caz, funcția sensor_get_humidity() returnează -1, fără a opri sistemul:
```c
if (raw == 0 || raw == 4095) {
    printf("WARN: senzor posibil deconectat (raw=%d)\n", raw);
    return -1;
}
```

Bucla principală ignoră citirile invalide și păstrează ultima valoare validă înregistrată, astfel încât display-ul și logica de udare să nu fie afectate în mod eronat:
```c
int val = sensor_get_humidity();
if (val >= 0) {
    umiditate = val;
} else {
    printf("WARN: citire senzor ignorata\n");
}
```

## Schema electrică a proiectului
![Schema electrica Smart Plant Watering](docs/imagine_schema.png)

### Setup Hardware
Aici este o imagine cu configurarea fizică a proiectului:

![Setup-ul proiectului](docs/poza_setup.jpg)

### Demonstrație Udare
Mai jos puteți urmări clipurile video cu procesul de udare automată:

#### Test Udare 1
[ Click aici pentru a vedea Clipul 1 (video_udare1.mp4)](docs/video_udare1.mp4)

#### Test Udare 2
[ Click aici pentru a vedea Clipul 2 (video_udare2.mp4)](docs/video_udare2.mp4)

### Schema software

![smartplant_software_bloc_core0](docs/smartplant_software_bloc_core0.svg)

![smartplant_software_bloc_core1_shared](docs/smartplant_software_bloc_core1_shared.svg)

> **Disclaimer:** Aceste scheme au fost realizate cu ajutorul inteligentei artificiale.
## Contribuțiile Membrilor Echipei
### Șalaru Ioana

* Documentație: redactarea README și descrierea arhitecturii software
* Logica principală (smart_plant.c): bucla non-blocantă pe Core 0, gestionarea timpilor prin time_us_64(), transmisia UART/USB în format JSON
* Modulul button: detecția apăsării prin întrerupere hardware GPIO, debounce software, logica de auto-selectare după 10 secunde
* Modulul sensor: citire ADC prin DMA cu medie mobilă pe 32 de sample-uri, calibrare, detecție senzor deconectat
* Modulul menu: mașina de stări pentru navigarea între profile, logica de confirmare profil și invocarea interfeței pe OLED
* Arhitectura dual-core: împărțirea responsabilităților între Core 0 și Core 1, mecanismul de comunicare prin SharedState protejat cu spinlock
* Conexiunile între Raspberry Pi Pico 2W, senzorul de umiditate, servomotor și display OLED
* Tratarea erorilor


### Panaite Elena-Alexandra

* Modulul ssd1306: driver complet I2C pentru display-ul OLED 128×64px, framebuffer, font 5×7, text normal și 2×, bară de progres
* Modulul servo: control PWM pentru servomotorul SG90, logica de udare non-blocantă cu cooldown de 30 de secunde
* Schema arhitecturii proiectului: diagrama bloc a componentelor software și hardware
* Schema electrică
* Documentare foto-video
* Prezentarea PowerPoint

## Bibliografie

[1] Raspberry Pi Ltd., "Raspberry Pi Pico 2 W Datasheet," Nov. 2024.
    [Online]. Available: https://datasheets.raspberrypi.com/picow/pico-2-w-datasheet.pdf

[2] Raspberry Pi Ltd., "RP2350 Datasheet," 2024.
    [Online]. Available: https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf

[3] Raspberry Pi Ltd., "Raspberry Pi Pico C/C++ SDK," 2024.
    [Online]. Available: https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf

[4] TowerPro, "SG90 9g Micro Servo Datasheet," Tower Pro Co., Ltd.
    [Online]. Available: http://www.handsontec.com/dataspecs/motor_fan/SG90-Servo.pdf

[5] Solomon Systech, "SSD1306 128x64 Dot Matrix OLED Driver with Controller," v1.1.
    [Online]. Available: https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf

[6] DFRobot / AZ-Delivery, "Capacitive Soil Moisture Sensor V1.2 (HW-101) Specification."
    [Online]. Available: https://www.datocms-assets.com/28969/1662716326-hw-101-hw-moisture-sensor-v1-0.pdf
