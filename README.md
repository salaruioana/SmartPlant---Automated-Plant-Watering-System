# SmartPlant: Sistem de monitorizare și irigare 

**Echipa BerryBusy:** Panaite Elena-Alexandra și Șalaru Ioana (Grupa 1309B)

---

## Descrierea Proiectului
SmartPlant este un sistem automatizat, bazat pe Raspberry Pi Pico 2w, creat pentru a monitoriza starea de sănătate a plantelor de apartament. Dispozitivul măsoară umiditatea solului și decide dacă este necesară irigarea. Interfața oferă feedback vizual printr-un display OLED, iar execuția irigării este simulată mecanic folosind un servomotor.

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
* Modularitate Software: Codul este dezvoltat în **C (Pico SDK)** și este modularizat (drivere separate pentru I2C, PWM, ADC).
* Eficiență și Timing: Utilizarea timerelor pentru eșantionarea senzorilor (arhitectură non-blocantă).
* Fiabilitate: Implementarea unor filtre software (medie mobilă) pe valorile preluate din ADC pentru a elimina zgomotul și a stabiliza deciziile de udare.

---

## Scenarii de Utilizare

**1. Configurarea inițială și selecția profilului**
* La pornirea dispozitivului, utilizatorul folosește butoanele pentru a naviga prin meniul afișat pe display-ul OLED. Selectează profilul „Cactus” (care necesită umiditate scăzută). Sistemul salvează pragul corespunzător și începe monitorizarea.

**2. Declanșarea irigării automate**
* Sistemul citește nivelul de umiditate din sol. Dacă valoarea scade sub pragul setat anterior, display-ul OLED afișează un mesaj: „Nivel Critic. Irigare în curs...”. Microcontrolerul generează un semnal PWM care rotește servomotorul la 90° pentru 5 secunde. Simultan, pe interfața Serială a PC-ului se trimite mesajul `[ALERT] Irigare declansata automat pt profilul Cactus`.
