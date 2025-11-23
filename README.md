# fan2go

**ESP32 PWM + RPM controller pro až 5× 4-pin PC ventilátorů.**  
Řízení přes USB (UART 115200), podporuje čtení otáček z tachometru a nastavení PWM.

---

### Funkce
- Ovládání až 5 ventilátorů
- PWM regulace 0–100 %
- Měření RPM pomocí tach signálu
- Detekce připojeného ventilátoru
- Ovládání přes USB příkazy
- Uložení a načtení konfigurace (WiFi, MQTT) do NVS
- Konfigurace přes USB (přes externí aplikaci)
- Připraveno pro budoucí integrace (WiFi, MQTT, Web UI)

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
| `GET FAN X` | Info o ventilátoru X | `FAN X Connected RPM Duty` |
| `GET ALL` | Stav všech fanů | `ALL mask rpm0 rpm1 … duty0 duty1 …` |
| `SET FAN X Y` | Nastavit X na Y % | `OK` nebo `ERR` |
| `SAVE WIFI ssid pass` | Uloží WiFi údaje do NVS | `OK` |
| `SAVE MQTT host client` | Uloží MQTT údaje do NVS | `OK` |

---

### Budoucí funkce
- Web UI přes HTTP
- MQTT (Home Assistant auto-discovery)
- Automatické řízení podle čidel
- Aktualizace parametrů OTA

---

### Licence
MIT

---

### Poznámka
Projekt je navržen pro Linux daemon **fan2go**, který bude sloužit jako most mezi ESP32 a Home Assistant / MQTT / Web konfigurací. NVS ukládání umožní bezpečné uchování WiFi a MQTT bez přítomnosti v `config.h`.

