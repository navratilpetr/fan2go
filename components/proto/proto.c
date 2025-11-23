#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "fan.h"
#include "storage.h"
#include "config.h"

static void fan_handle_command(const char *line)
{
    /* SET FAN idx duty */
    if (strncmp(line, "SET FAN ", 8) == 0) {
        int idx, duty;
        if (sscanf(line + 8, "%d %d", &idx, &duty) == 2 && idx >= 0) {
            fan_set_duty_percent(fan_get_ptr(idx), duty);
            printf("OK\n");
        } else {
            printf("ERR\n");
        }
        return;
    }

    /* GET FAN idx */
    if (strncmp(line, "GET FAN ", 8) == 0) {
        int idx;
        if (sscanf(line + 8, "%d", &idx) == 1) {
            printf("FAN %d %d %" PRIu32 " %d\n",
                   idx,
                   fan_is_connected(idx) ? 1 : 0,
                   fan_get_rpm(idx),
                   fan_get_duty(idx));
        } else {
            printf("ERR\n");
        }
        return;
    }

    /* GET ALL */
    if (strcmp(line, "GET ALL") == 0) {
        uint8_t mask = 0;
        for (int i = 0; i < CONFIG_MAX_FANS; i++)
            if (fan_is_connected(i)) mask |= (1 << i);

        printf("ALL 0x%02X", mask);
        for (int i = 0; i < CONFIG_MAX_FANS; i++)
            printf(" %" PRIu32, fan_get_rpm(i));
        for (int i = 0; i < CONFIG_MAX_FANS; i++)
            printf(" %d", fan_get_duty(i));
        printf("\n");
        return;
    }

    /* PING */
    if (strcmp(line, "PING") == 0) {
        printf("PONG\n");
        return;
    }

    /* SET WIFI ssid pass */
    if (strncmp(line, "SET WIFI ", 9) == 0) {
        char ssid[32];
        char pass[64];

        if (sscanf(line + 9, "%31s %63s", ssid, pass) == 2) {
            wifi_conf_t cfg;
            memset(&cfg, 0, sizeof(cfg));
            strncpy(cfg.ssid, ssid, sizeof(cfg.ssid) - 1);
            strncpy(cfg.pass, pass, sizeof(cfg.pass) - 1);
            storage_set_wifi(&cfg);
            printf("OK\n");
        } else {
            printf("ERR\n");
        }
        return;
    }

    /* GET WIFI */
    if (strcmp(line, "GET WIFI") == 0) {
        wifi_conf_t cfg;
        storage_get_wifi(&cfg);
        printf("WIFI %s %s\n", cfg.ssid, cfg.pass);
        return;
    }

    /* SET MQTT host clientid (port necháme 1883 nebo co je v NVS) */
    if (strncmp(line, "SET MQTT ", 9) == 0) {
        char host[64];
        char cid[64];

        if (sscanf(line + 9, "%63s %63s", host, cid) == 2) {
            mqtt_conf_t cfg;
            storage_get_mqtt(&cfg);  /* zachová port */
            strncpy(cfg.host, host, sizeof(cfg.host) - 1);
            cfg.host[sizeof(cfg.host) - 1] = 0;
            strncpy(cfg.client_id, cid, sizeof(cfg.client_id) - 1);
            cfg.client_id[sizeof(cfg.client_id) - 1] = 0;
            storage_set_mqtt(&cfg);
            printf("OK\n");
        } else {
            printf("ERR\n");
        }
        return;
    }

    /* GET MQTT */
    if (strcmp(line, "GET MQTT") == 0) {
        mqtt_conf_t cfg;
        storage_get_mqtt(&cfg);
        printf("MQTT %s %d %s\n", cfg.host, cfg.port, cfg.client_id);
        return;
    }

    /* unknown */
    printf("ERR\n");
}

static void fan_console_task(void *arg)
{
    char line[128];

    setvbuf(stdin,  NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    while (1) {
        if (!fgets(line, sizeof(line), stdin)) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == '\n'))
            line[--len] = 0;

        if (len < 3) continue;

        fan_handle_command(line);
    }
}

void proto_init(void)
{
    xTaskCreate(fan_console_task, "fan_console", 4096, NULL, 4, NULL);
}

