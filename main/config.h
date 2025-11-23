#pragma once

/* === Fan count === */
#define CONFIG_MAX_FANS 5

/* === PWM pins === */
#define FAN_PWM_0 23
#define FAN_PWM_1 19
#define FAN_PWM_2 18
#define FAN_PWM_3 5
#define FAN_PWM_4 4

/* === Tach pins === */
#define FAN_TAC_0 32
#define FAN_TAC_1 33
#define FAN_TAC_2 25
#define FAN_TAC_3 26
#define FAN_TAC_4 27

/* === PWM config === */
#define FAN_PWM_FREQ_HZ       25000
#define FAN_PWM_RES_BITS      8

/* === Tach config === */
#define FAN_PULSES_PER_REV    2
#define FAN_DETECT_MS         1000
#define FAN_DETECT_MIN_PULSES 5
#define FAN_RPM_INTERVAL_MS   1000

/* === Default duty % === */
#define FAN_DEFAULT_DUTY      50

/* === Reserved for future WiFi/MQTT === */
#define WIFI_SSID     "SSID"
#define WIFI_PASS     "PASSWORD"
#define MQTT_HOST     "mqtt.local"
#define MQTT_CLIENTID "fan2go"

