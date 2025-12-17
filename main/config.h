#pragma once

/* === FAN COUNT === */
#define CONFIG_MAX_FANS 5

/* === PWM PINS === */
#define FAN_PWM_0 23
#define FAN_PWM_1 19
#define FAN_PWM_2 18
#define FAN_PWM_3 5
#define FAN_PWM_4 4

/* === TACH PINS === */
#define FAN_TAC_0 32
#define FAN_TAC_1 33
#define FAN_TAC_2 25
#define FAN_TAC_3 26
#define FAN_TAC_4 27

/* === PWM CONFIG === */
#define FAN_PWM_FREQ_HZ       25000
#define FAN_PWM_RES_BITS      8

/* === RPM / DETECT CONFIG === */
#define FAN_PULSES_PER_REV    2
#define FAN_DETECT_MS         1000
#define FAN_DETECT_MIN_PULSES 5
#define FAN_RPM_INTERVAL_MS   1000

/* === FAN CALIBRATION ENABLE === */
/* 1 = enable autocalibration on boot */
/* 0 = disable calibration completely */
#define FAN_ENABLE_CALIBRATION 0


/* === AUTOCALIBRATION === */
/* Step size for increasing PWM percentage (x % per step) */
#define FAN_CAL_STEP_PERCENT  1
/* Delay after each step to allow RPM stabilization (ms) */
#define FAN_CAL_SETTLE_MS     1000

/* === FALLBACK WATCHDOG === */
/* If backend sends no UART commands for X ms, force safe fan speed */
#define FAN_FALLBACK_MS       120000
/* Safe fallback speed after timeout (scaled %, 0–100) */
#define FAN_FALLBACK_DUTY     100

/* === DEFAULT DUTY (only used before calibration) === */
#define FAN_DEFAULT_DUTY      50

/* === WiFi DEFAULT (if NVS empty) === */
#define WIFI_SSID     "ssid"
#define WIFI_PASS     "password"

/* === MQTT DEFAULT (if NVS empty) === */
#define MQTT_HOST     "mqtt://192.168.2.11"
#define MQTT_PORT     1883
#define MQTT_CLIENTID "fan2go"

/* === MQTT AUTH === */
#define MQTT_USER     "user"
#define MQTT_PASS     "passwd"

/* === MQTT DISCOVERY === */
#define HA_DISCOVERY_PREFIX "homeassistant"
#define HA_DEVICE_ID        "fan2go"
#define HA_DEVICE_NAME      "ESP Fan Controller"


/* === MQTT TOPICS === */
#define MQTT_BASE_TOPIC         "fan2go"
#define MQTT_CMD_FAN_TEMPLATE   "fan2go/cmd/fan/%d/set"   /* payload: 0–100 */
#define MQTT_CMD_ALL_TOPIC      "fan2go/cmd/all/set"      /* payload: 0–100 */
#define MQTT_STATE_FAN_TEMPLATE "fan2go/state/fan/%d"     /* payload: connected rpm duty */
#define MQTT_STATE_ALL_TOPIC    "fan2go/state/all"        /* payload: all fans */

