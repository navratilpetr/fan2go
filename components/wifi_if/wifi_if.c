#include <string.h>
#include <stdio.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_err.h"

#include "config.h"
#include "storage.h"
#include "wifi_if.h"

static bool s_wifi_started = false;

void wifi_init(void)
{
    if (s_wifi_started) {
        return;
    }
    s_wifi_started = true;

    wifi_conf_t cfg;
    storage_get_wifi(&cfg);

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_cfg);

    esp_wifi_set_mode(WIFI_MODE_STA);

    wifi_config_t wifi_config = { 0 };
    strncpy((char *)wifi_config.sta.ssid,     cfg.ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, cfg.pass, sizeof(wifi_config.sta.password) - 1);

    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();

    printf("WiFi: STA init, SSID=\"%s\"\n", cfg.ssid);
}

