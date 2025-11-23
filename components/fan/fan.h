#pragma once
#include <stdint.h>
#include <stdbool.h>

void fan_init_all(void);
void fan_set_duty_percent(void *fan_ptr, int duty_percent);
void *fan_get_ptr(int idx);

uint32_t fan_get_rpm(int idx);
bool     fan_is_connected(int idx);
int      fan_get_duty(int idx);

