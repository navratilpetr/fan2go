#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mqtt_client.h"
#include "config.h"
#include "fan.h"
#include "storage.h"
#include "mqtt_if.h"

static esp_mqtt_client_handle_t s_client = NULL;
static bool s_mqtt_connected = false;

static void mqtt_publish_state_all(void);

static void mqtt_on_data(const esp_mqtt_event_handle_t event)
{
    char topic[128];
    char data[128];

    int tlen = event->topic_len;
    if (tlen >= (int)sizeof(topic)) tlen = sizeof(topic) - 1;
    memcpy(topic, event->topic, tlen);
    topic[tlen] = '\0';

    int dlen = event->data_len;
    if (dlen >= (int)sizeof(data)) dlen = sizeof(data) - 1;
    memcpy(data, event->data, dlen);
    data[dlen] = '\0';

    /* Per-fan: fan2go/cmd/fan/<idx>/set */
    int idx = -1;
    if (sscanf(topic, "fan2go/cmd/fan/%d/set", &idx) == 1) {
        int duty = atoi(data);
        if (idx >= 0 && idx < CONFIG_MAX_FANS) {
            fan_set_duty_percent(fan_get_ptr(idx), duty);
            printf("MQTT: set fan %d duty %d\n", idx, duty);
            mqtt_publish_state_all();
        } else {
            printf("MQTT: invalid fan idx %d\n", idx);
        }
        return;
    }

    /* All-fans: fan2go/cmd/all/set */
    if (strcmp(topic, "fan2go/cmd/all/set") == 0) {
        int duty = atoi(data);
        printf("MQTT: set ALL fans duty %d\n", duty);
        for (int i = 0; i < CONFIG_MAX_FANS; ++i) {
            fan_set_duty_percent(fan_get_ptr(i), duty);
        }
        mqtt_publish_state_all();
        return;
    }
}

static void mqtt_event_handler(void *handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        s_mqtt_connected = true;
        printf("MQTT: connected\n");
        esp_mqtt_client_subscribe(s_client, "fan2go/cmd/fan/+/set", 0);
        esp_mqtt_client_subscribe(s_client, "fan2go/cmd/all/set", 0);
        mqtt_publish_state_all();
        break;

    case MQTT_EVENT_DISCONNECTED:
        s_mqtt_connected = false;
        printf("MQTT: disconnected\n");
        break;

    case MQTT_EVENT_DATA:
        mqtt_on_data(event);
        break;

    default:
        break;
    }
}

static void mqtt_state_task(void *arg)
{
    const TickType_t delay = pdMS_TO_TICKS(5000);

    while (1) {
        if (s_mqtt_connected) {
            mqtt_publish_state_all();
        }
        vTaskDelay(delay);
    }
}

static void mqtt_publish_state_all(void)
{
    if (!s_mqtt_connected || s_client == NULL) {
        return;
    }

    char buf[256];
    uint8_t mask = 0;

    for (int i = 0; i < CONFIG_MAX_FANS; ++i) {
        if (fan_is_connected(i)) {
            mask |= (1u << i);
        }
    }

    int len = snprintf(buf, sizeof(buf), "ALL 0x%02X", mask);
    for (int i = 0; i < CONFIG_MAX_FANS; ++i) {
        len += snprintf(buf + len, sizeof(buf) - len, " %" PRIu32, fan_get_rpm(i));
    }
    for (int i = 0; i < CONFIG_MAX_FANS; ++i) {
        len += snprintf(buf + len, sizeof(buf) - len, " %d", fan_get_duty(i));
    }

    esp_mqtt_client_publish(s_client, "fan2go/state/all", buf, 0, 0, 0);

    for (int i = 0; i < CONFIG_MAX_FANS; ++i) {
        char topic[64];
        snprintf(topic, sizeof(topic), "fan2go/state/fan/%d", i);
        char fb[64];
        snprintf(fb, sizeof(fb), "%d %" PRIu32 " %d",
                 fan_is_connected(i) ? 1 : 0,
                 fan_get_rpm(i),
                 fan_get_duty(i));
        esp_mqtt_client_publish(s_client, topic, fb, 0, 0, 0);
    }
}

void mqtt_init(void)
{
    if (s_client != NULL) {
        return;
    }

    mqtt_conf_t cfg;
    storage_get_mqtt(&cfg);

    esp_mqtt_client_config_t mc = {
        .broker.address.hostname = cfg.host,
        .broker.address.port     = cfg.port,
        .credentials.client_id   = cfg.client_id,
    };

    s_client = esp_mqtt_client_init(&mc);
    if (!s_client) {
        printf("MQTT: client init failed\n");
        return;
    }

    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(s_client);

    xTaskCreate(mqtt_state_task, "mqtt_state", 4096, NULL, 4, NULL);
}

