#pragma once
#include <stdint.h>
#include <stdbool.h>

void fan_init_all(void);
void *fan_get_ptr(int idx);
void fan_set_duty_percent(void *fan_ptr, int duty_percent);

/* Public getters */
uint32_t fan_get_rpm(int idx);
bool     fan_is_connected(int idx);
int      fan_get_duty(int idx);

/* Calibration */
int  fan_get_min_start(int idx);      /* returns calibrated min duty */
void fan_calibrate_all(void);         /* runs async calibration */

/* Fallback refresh (called when backend sends command) */
void fan_backend_alive(void);

