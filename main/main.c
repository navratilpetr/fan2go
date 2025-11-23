#include <stdio.h>

#include "storage.h"
#include "fan.h"
#include "proto.h"
#include "wifi_if.h"
#include "mqtt_if.h"
#include "web_if.h"

void app_main(void)
{
    /* NVS (pro WiFi/MQTT konfiguraci) */
    storage_init();

    /* WiFi (SSID/heslo z NVS nebo z config.h) */
    wifi_init();

    /* Ventilátory */
    fan_init_all();

    /* UART protokol (fan2go) */
    proto_init();

    /* MQTT (config z NVS / config.h) */
    mqtt_init();

    /* Web rozhraní */
    web_init();

    printf("READY\n");
}

