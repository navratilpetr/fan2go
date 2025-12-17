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

/* ===== Globals ===== */
static esp_mqtt_client_handle_t s_client = NULL;
static bool s_mqtt_connected = false;

/* ===== Prototypes ===== */
static void mqtt_publish_state_all(void);
static void mqtt_publish_discovery(void);

/* ===== MQTT event handler ===== */
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

        mqtt_publish_discovery();
        mqtt_publish_state_all();
        break;

    case MQTT_EVENT_DISCONNECTED:
        s_mqtt_connected = false;
        printf("MQTT: disconnected\n");
        break;

    default:
        break;
    }
}

/* ===== Periodic state publisher ===== */
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

/* ===== Publish fan states =====
 * fan2go/state/fan/X
 * payload: "<connected> <rpm> <duty>"
 */
static void mqtt_publish_state_all(void)
{
    if (!s_mqtt_connected || !s_client) return;

    char topic[64];
    char payload[64];

    for (int i = 0; i < CONFIG_MAX_FANS; i++) {
        snprintf(topic, sizeof(topic),
                 "fan2go/state/fan/%d", i);

        snprintf(payload, sizeof(payload),
                 "%d %" PRIu32 " %d",
                 fan_is_connected(i) ? 1 : 0,
                 fan_get_rpm(i),
                 fan_get_duty(i));

        esp_mqtt_client_publish(s_client, topic, payload, 0, 0, 1);
    }
}

/* ===== Home Assistant MQTT Discovery ===== */
static void mqtt_publish_discovery(void)
{
    char topic[256];
    char payload[512];

    for (int i = 0; i < CONFIG_MAX_FANS; i++) {

        /* ---------- RPM sensor ---------- */
        snprintf(topic, sizeof(topic),
            "%s/sensor/%s_fan%d_rpm/config",
            HA_DISCOVERY_PREFIX, HA_DEVICE_ID, i);

        snprintf(payload, sizeof(payload),
            "{"
              "\"name\":\"Fan %d RPM\","
              "\"state_topic\":\"fan2go/state/fan/%d\","
              "\"value_template\":\"{{ value.split(' ')[1] }}\","
              "\"unit_of_measurement\":\"RPM\","
              "\"unique_id\":\"%s_fan%d_rpm_v2\","
              "\"device\":{"
                "\"identifiers\":[\"%s\"],"
                "\"name\":\"%s\","
                "\"manufacturer\":\"fan2go\","
                "\"model\":\"ESP32 Fan Controller\""
              "}"
            "}",
            i, i,
            HA_DEVICE_ID, i,
            HA_DEVICE_ID, HA_DEVICE_NAME);

        esp_mqtt_client_publish(s_client, topic, payload, 0, 0, 1);

        /* ---------- DUTY sensor ---------- */
        snprintf(topic, sizeof(topic),
            "%s/sensor/%s_fan%d_duty/config",
            HA_DISCOVERY_PREFIX, HA_DEVICE_ID, i);

        snprintf(payload, sizeof(payload),
            "{"
              "\"name\":\"Fan %d Duty\","
              "\"state_topic\":\"fan2go/state/fan/%d\","
              "\"value_template\":\"{{ value.split(' ')[2] }}\","
              "\"unit_of_measurement\":\"%%\","
              "\"unique_id\":\"%s_fan%d_duty_v2\","
              "\"device\":{"
                "\"identifiers\":[\"%s\"],"
                "\"name\":\"%s\","
                "\"manufacturer\":\"fan2go\","
                "\"model\":\"ESP32 Fan Controller\""
              "}"
            "}",
            i, i,
            HA_DEVICE_ID, i,
            HA_DEVICE_ID, HA_DEVICE_NAME);

        esp_mqtt_client_publish(s_client, topic, payload, 0, 0, 1);
    }
}


/* ===== MQTT init ===== */
void mqtt_init(void)
{
    if (s_client) return;

    mqtt_conf_t cfg;
    storage_get_mqtt(&cfg);

    esp_mqtt_client_config_t mc = {
        .broker.address.uri = cfg.host,   // <-- DŮLEŽITÉ
        .credentials = {
            .client_id = cfg.client_id,
            .username  = MQTT_USER,
            .authentication.password = MQTT_PASS,
        },
    };

    s_client = esp_mqtt_client_init(&mc);
    if (!s_client) {
        printf("MQTT: client init failed\n");
        return;
    }

    esp_mqtt_client_register_event(
        s_client,
        ESP_EVENT_ANY_ID,
        mqtt_event_handler,
        NULL
    );

    esp_mqtt_client_start(s_client);

    xTaskCreate(mqtt_state_task, "mqtt_state", 4096, NULL, 4, NULL);
}

