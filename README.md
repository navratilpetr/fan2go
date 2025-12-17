# fan2go

ESP32 PWM + RPM controller pro až 5× 4-pin PC ventilátorů.  
Řízení přes USB (UART 115200), měření otáček z tachometru, MQTT monitoring a Home Assistant auto-discovery.

---

###FUNKCE
- Ovládání až 5 ventilátorů (fan0–fan4)
- PWM řízení 0–100 % (interně ESP32)
- Měření RPM přes tachometr
- Detekce připojeného ventilátoru
- Volitelná autokalibrace minimálních otáček
- Fallback bezpečné otáčky při ztrátě komunikace
- USB protokol (UART 115200)
- MQTT pouze pro monitoring (read-only)
- Home Assistant MQTT Discovery
- Web UI (stav / test)
- Ukládání WiFi a MQTT konfigurace do NVS

---

###PWM A ROZSAHY
- ESP32 pracuje s PWM v rozsahu 0–100 %
- Linux hwmon používá rozsah 0–255
- Přepočet 0–255 ↔ 0–100 zajišťuje USB bridge
- Toto chování je záměrné a správné

---

###AUTOKALIBRACE
Autokalibrace slouží ke zjištění minimální hodnoty PWM, při které se ventilátor skutečně roztočí.

Princip:
1. PWM se nastaví na 0 %
2. Postupně se zvyšuje po malých krocích
3. Po každém kroku se čeká na ustálení otáček
4. Jakmile RPM poprvé smysluplně vzrostou, hodnota se uloží jako minimum
5. Při běžném provozu se nízké hodnoty automaticky přepočítají nad toto minimum

Autokalibraci lze zcela vypnout v config.h:

#define FAN_ENABLE_CALIBRATION 0   // 0 = vypnuto, 1 = zapnuto

Doporučeno vypnout u ventilátorů, které při nízkých otáčkách reportují nesmyslné RPM.

---

###FALLBACK (BEZPECNOST)
Pokud ESP po definovanou dobu nedostane žádný příkaz z backendu (USB / MQTT), automaticky nastaví bezpečné otáčky.

Nastavení v config.h:

#define FAN_FALLBACK_MS    120000
#define FAN_FALLBACK_DUTY  100

---

###PINY (main/config.h)

Fan 0: PWM 23, TACH 32  
Fan 1: PWM 19, TACH 33  
Fan 2: PWM 18, TACH 25  
Fan 3: PWM 5,  TACH 26  
Fan 4: PWM 4,  TACH 27  

---

### USB protokol (UART 115200) 
| Příkaz | Popis | Odpověď | 
|--------|-------|---------| 
| PING | Test komunikace | PONG | 
| SET FAN X Y | Nastavit fan X na Y % | OK nebo ERR | 
| GET FAN X | Info o ventilátoru X | FAN X Connected RPM Duty | 
| GET ALL | Stav všech fanů | ALL mask rpm... duty... | 
| GET RPM X | Vrátí RPM fan X | RPM X value | 
| GET DUTY X | Vrátí PWM fan X (%) | DUTY X value | 
| SET WIFI ssid pass | Uloží WiFi údaje do NVS | OK | 
| GET WIFI | Vrátí uložené WiFi údaje | WIFI ssid pass | 
| SET MQTT host clientid | Uloží MQTT do NVS | OK | 
| GET MQTT | Vrátí uložené MQTT údaje | MQTT host port clientid |

---

###MQTT
- MQTT je pouze pro monitoring
- Nenastavuje PWM
- Publikuje RPM, duty a stav připojení
- Retain = zapnuto
- Automatická Home Assistant discovery

V Home Assistant se vytvoří:
- Jedno zařízení (ESP Fan Controller)
- Pro každý ventilátor:
  - Fan X RPM
  - Fan X Duty

---

###LINUX INTEGRACE
Projekt počítá s Linux backendem:
- kernel modul espfan (hwmon)
- userspace USB bridge
- kompatibilní se sensors, fancontrol a Home Assistant

---

LICENCE
MIT

---

###POZNAMKA
ESP32 řeší real-time část (PWM, RPM, bezpečnost).  
Linux / Home Assistant řeší logiku, monitoring a automatizace.  
Projekt je navržen jako stabilní backend, ne jako experiment.

