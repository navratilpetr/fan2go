#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "config.h"
#include "fan.h"

static void fan_handle_command(const char *line)
{
    if (strncmp(line, "SET FAN ", 8) == 0) {
        int idx, duty;
        if (sscanf(line + 8, "%d %d", &idx, &duty) == 2 &&
            idx >= 0 && idx < CONFIG_MAX_FANS) {
            fan_set_duty_percent(fan_get_ptr(idx), duty);
            printf("OK\n");
        } else {
            printf("ERR\n");
        }
        return;
    }

    if (strncmp(line, "GET FAN ", 8) == 0) {
        int idx;
        if (sscanf(line + 8, "%d", &idx) == 1 &&
            idx >= 0 && idx < CONFIG_MAX_FANS) {
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

    if (strcmp(line, "PING") == 0) {
        printf("PONG\n");
        return;
    }

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

        if (len < 3)
            continue;

        fan_handle_command(line);
    }
}

void proto_init(void)
{
    xTaskCreate(fan_console_task, "fan_console", 4096, NULL, 4, NULL);
}

