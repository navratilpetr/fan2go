# fan2go

ESP32 PWM + RPM controller for up to 5 PC fans (4-pin PWM + tach).

### Protocol (UART 115200)

| Command | Description |
|--------|-------------|
| `PING` | Responds `PONG` |
| `GET FAN X` | Returns connection, RPM, duty |
| `GET ALL` | Returns mask + RPM + duty |
| `SET FAN X Y` | Set duty % |

### TODO Features
- WiFi
- MQTT (Home Assistant)
- Web monitoring
- NVS duty save/load

---

Made for Linux control scripts (fan2go daemon) and ESP32 embedded use.

