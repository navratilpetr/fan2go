# fan2go

**ESP32 PWM + RPM controller pro až 5× 4-pin PC ventilátorů.**  
Řízení přes USB (UART 115200), podporuje čtení otáček z tachometru a nastavení PWM.

---

### ✨ Funkce
- Ovládání až 5 ventilátorů
- PWM regulace 0–100 %
- Měření RPM pomocí tach signálu
- Detekce připojeného ventilátoru
- Automatická kalibrace minimálních otáček
- Fallback ochrana při ztrátě komunikace (bezpečná rychlost)
- Ovládání přes USB příkazy
- Uložení a načtení konfigurace (WiFi, MQTT) do NVS
- Připraveno pro integraci s Linux daemonem **fan2go**
- Připraveno pro Web UI / WiFi / MQTT Home Assistant

---

### 📌 Piny (nastavitelné v `main/config.h`)
| Fan | PWM GPIO | Tach GPIO |
|-----|----------|-----------|
|  0  |    23    |    32     |
|  1  |    19    |    33     |
|  2  |    18    |    25     |
|  3  |     5    |    26     |
|  4  |     4    |    27     |

> **Poznámka:** Tach signál musí být připojen přes pull-up (většina ventilátorů má interní).

---

### 🔌 USB protokol (UART 115200)

| Příkaz | Popis | Odpověď |
|--------|-------|---------|
| `PING` | Test komunikace | `PONG` |
| `GET FAN X` | Info o ventilátoru X | `FAN X Connected RPM Duty` |
| `GET ALL` | Stav všech fanů | `ALL mask rpm0 rpm1 … duty0 duty1 …` |
| `SET FAN X Y` | Nastavit X na Y % | `OK` / `ERR` |
| `SET WIFI ssid pass` | Uloží WiFi údaje do NVS | `OK` |
| `GET WIFI` | Načte WiFi údaje | `WIFI ssid pass` |
| `SET MQTT host client` | Uloží MQTT údaje do NVS | `OK` |
| `GET MQTT` | Načte MQTT údaje | `MQTT host port client` |

**Maska v `GET ALL`:** bit = 1 znamená připojený ventilátor.

---

### 🔧 Automatická kalibrace ventilátorů

Firmware při startu provádí **automatickou kalibraci minimálních otáček každého ventilátoru**, aby byla regulace přesná pro různé typy.

#### Proč je to nutné?
Každý ventilátor se umí točit i na 0 % PWM, ale RPM se **nezvyšuje**, dokud PWM nepřekročí určitou hranici (odlišnou pro každý kus). Příklady:

| Ventilátor | Hodnota, při které začne zvyšovat RPM |
|-------------|--------------------------------------|
| Fan A | 25 % |
| Fan B | 40 % |
| Fan C | 15 % |

Bez kalibrace by 20 % PWM znamenalo různou reálnou rychlost.

#### Jak kalibrace funguje
1. PWM = **0 %**, čekání na ustálení.
2. Zvyšování PWM **po krocích (`FAN_CAL_STEP_PERCENT`)**, obvykle po 5 %.
3. Po každém kroku čekáme **`FAN_CAL_SETTLE_MS` ms**, aby se ventilátor ustálil.
4. Jakmile se RPM **zvýší o ≥ 50 RPM** oproti předchozímu kroku, tato hodnota PWM se uloží jako **minimální regulovatelný bod**.
5. Po kalibraci se PWM opět nastaví na **0 %**.

#### Výstup kalibrace (příklad)
CALIB: fan0 min duty=15%
CALIB: fan1 min duty=25%
CALIB: fan2 min duty=10%


#### Co se děje při regulaci po kalibraci
- Uživatelských **0–100 %** se přemapuje:
  - `0 %` = ventilátor stojí
  - `1–100 %` = lineární škála mezi `min_duty` a `100 % PWM`
- U různých ventilátorů tak platí **stejná charakteristika regulace**.

---

### 🛡️ Fallback ochrana (bezpečné otáčky)

Pokud ESP32 **nedostane žádný příkaz z backendu** (USB) po dobu `FAN_FALLBACK_MS`, firmware automaticky nastaví všechny připojené ventilátory na **bezpečnou hodnotu `FAN_FALLBACK_DUTY`**.

> To chrání zařízení při pádu serveru nebo odpojení USB.

---

### 📦 Vývoj a integrace

Projekt je navržen jako součást ekosystému:
- ESP32 = hardware řízení ventilátorů
- Linux daemon **fan2go** = propojení s Home Assistant / MQTT / Web konfigurací
- NVS ukládá WiFi a MQTT, což umožňuje OTA konfiguraci bez zásahu do firmware

---

### 📜 Licence
MIT

---

Pokud chceš doplnit schémata zapojení, obrázky web UI nebo Home Assistant auto-discovery, napiš.

