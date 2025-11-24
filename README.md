# fan2go

**ESP32 PWM + RPM controller pro až 5× 4-pin PC ventilátorů.**  
Řízení přes USB (UART 115200), podporuje čtení otáček z tachometru, autokalibraci a nastavení PWM.

---

### Funkce
- Ovládání až 5 ventilátorů
- PWM regulace 0–100 %
- Měření RPM pomocí tachometru
- Detekce připojeného ventilátoru
- **Autokalibrace min. otáček při startu**
- **Fallback bezpečné otáčky, pokud PC nekomunikuje**
- Ovládání přes USB příkazy
- Uložení a načtení konfigurace (WiFi, MQTT) do NVS
- Web UI, MQTT (připraveno)

---

### Autokalibrace (jak funguje)
Každý ventilátor má jiné minimum, při kterém začnou růst otáčky.  
ESP32 automaticky zjistí toto minimum:

1. Ventilátor se začne od 0 % a postupně se zvyšuje po **nastaveném kroku (např. 5 %)**.
2. Po každém kroku počká na ustálení otáček.
3. Jakmile se RPM poprvé zvýší > 0, tato hodnota se uloží jako **minimální duty**.
4. V dalším provozu když nastavíte nižší hodnotu než minimum, automaticky se použije tato kalibrovaná hodnota.

Díky tomu je škála PWM **0–100 % vždy plynulá**, i kdyby se ventilátor reálně rozbíhal např. teprve při 42 %.

---

### Fallback (bezpečnost)
Pokud ESP **nedostane žádný příkaz z PC po dobu X sekund**, nastaví bezpečnou hodnotu (výchozí 50 %).

Parametry se nastavují v `config.h`:

#define FAN_FALLBACK_MS 15000
#define FAN_FALLBACK_DUTY 50


---

### Piny (nastavitelné v `main/config.h`)
| Fan | PWM GPIO | Tach GPIO |
|-----|----------|-----------|
|  0  |    23    |    32     |
|  1  |    19    |    33     |
|  2  |    18    |    25     |
|  3  |     5    |    26     |
|  4  |     4    |    27     |

---

### USB protokol (UART 115200)

| Příkaz | Popis | Odpověď |
|--------|-------|---------|
| `PING` | Test komunikace | `PONG` |
| `SET FAN X Y` | Nastavit fan X na Y % | `OK` nebo `ERR` |
| `GET FAN X` | Info o ventilátoru X | `FAN X Connected RPM Duty` |
| `GET ALL` | Stav všech fanů | `ALL mask rpm... duty...` |
| `GET RPM X` | Vrátí RPM fan X | `RPM X value` |
| `GET DUTY X` | Vrátí PWM fan X (%) | `DUTY X value` |
| `SET WIFI ssid pass` | Uloží WiFi údaje do NVS | `OK` |
| `GET WIFI` | Vrátí uložené WiFi údaje | `WIFI ssid pass` |
| `SET MQTT host clientid` | Uloží MQTT do NVS | `OK` |
| `GET MQTT` | Vrátí uložené MQTT údaje | `MQTT host port clientid` |

---

### Licence
MIT

---

### Poznámka
Projekt je navržen pro Linux daemon **fan2go**, který bude sloužit jako most mezi ESP32 a Home Assistant / MQTT / Web konfigurací.  
NVS ukládání umožní bezpečné uchování WiFi a MQTT bez přítomnosti v `config.h`.


