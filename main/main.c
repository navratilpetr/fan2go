#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "fan.h"
#include "proto.h"

void app_main(void)
{
    nvs_flash_init();

    fan_init_all();
    proto_init();

    printf("READY\n");
}

