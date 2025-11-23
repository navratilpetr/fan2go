#pragma once
#include <stdint.h>

typedef struct {
    char ssid[32];
    char pass[64];
} wifi_conf_t;

typedef struct {
    char host[64];
    int  port;
    char client_id[64];
} mqtt_conf_t;

void storage_init(void);

void storage_get_wifi(wifi_conf_t *out);
void storage_set_wifi(const wifi_conf_t *cfg);

void storage_get_mqtt(mqtt_conf_t *out);
void storage_set_mqtt(const mqtt_conf_t *cfg);

