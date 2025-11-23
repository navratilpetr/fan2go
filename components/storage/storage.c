#include <string.h>
#include <stdio.h>

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_err.h"

#include "config.h"
#include "storage.h"

#define NVS_NAMESPACE "cfg"

void storage_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
}

/* Helper: safe strncpy + terminace */
static void safe_strcpy(char *dst, size_t dst_size, const char *src)
{
    if (dst_size == 0) return;
    if (!src) {
        dst[0] = 0;
        return;
    }
    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = 0;
}

void storage_get_wifi(wifi_conf_t *out)
{
    if (!out) return;

    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        /* fallback na config.h */
        safe_strcpy(out->ssid, sizeof(out->ssid), WIFI_SSID);
        safe_strcpy(out->pass, sizeof(out->pass), WIFI_PASS);
        return;
    }

    /* pokus o načtení */
    size_t len_ssid = sizeof(out->ssid);
    size_t len_pass = sizeof(out->pass);
    bool have_ssid = (nvs_get_str(h, "wifi_ssid", out->ssid, &len_ssid) == ESP_OK);
    bool have_pass = (nvs_get_str(h, "wifi_pass", out->pass, &len_pass) == ESP_OK);

    if (!have_ssid || !have_pass) {
        /* NVS je prázdné nebo nekompletní → použij config.h a ulož do NVS */
        safe_strcpy(out->ssid, sizeof(out->ssid), WIFI_SSID);
        safe_strcpy(out->pass, sizeof(out->pass), WIFI_PASS);

        nvs_set_str(h, "wifi_ssid", out->ssid);
        nvs_set_str(h, "wifi_pass", out->pass);
        nvs_commit(h);
    }

    nvs_close(h);
}

void storage_set_wifi(const wifi_conf_t *cfg)
{
    if (!cfg) return;

    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) return;

    nvs_set_str(h, "wifi_ssid", cfg->ssid);
    nvs_set_str(h, "wifi_pass", cfg->pass);
    nvs_commit(h);
    nvs_close(h);
}

void storage_get_mqtt(mqtt_conf_t *out)
{
    if (!out) return;

    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        /* fallback na config.h + default port 1883 */
        safe_strcpy(out->host, sizeof(out->host), MQTT_HOST);
        out->port = 1883;
        safe_strcpy(out->client_id, sizeof(out->client_id), MQTT_CLIENTID);
        return;
    }

    size_t len_host = sizeof(out->host);
    size_t len_cid  = sizeof(out->client_id);
    int32_t port    = 0;

    bool have_host = (nvs_get_str(h, "mqtt_host", out->host, &len_host) == ESP_OK);
    bool have_cid  = (nvs_get_str(h, "mqtt_cid",  out->client_id, &len_cid) == ESP_OK);
    bool have_port = (nvs_get_i32(h, "mqtt_port", &port) == ESP_OK);

    if (!have_host || !have_cid || !have_port) {
        /* NVS prázdné → použij config.h + default port, a rovnou ulož */
        safe_strcpy(out->host, sizeof(out->host), MQTT_HOST);
        out->port = 1883;
        safe_strcpy(out->client_id, sizeof(out->client_id), MQTT_CLIENTID);

        nvs_set_str(h, "mqtt_host", out->host);
        nvs_set_str(h, "mqtt_cid",  out->client_id);
        nvs_set_i32(h, "mqtt_port", out->port);
        nvs_commit(h);
    } else {
        out->port = port;
    }

    nvs_close(h);
}

void storage_set_mqtt(const mqtt_conf_t *cfg)
{
    if (!cfg) return;

    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) return;

    nvs_set_str(h, "mqtt_host", cfg->host);
    nvs_set_str(h, "mqtt_cid",  cfg->client_id);
    nvs_set_i32(h, "mqtt_port", cfg->port);
    nvs_commit(h);
    nvs_close(h);
}

