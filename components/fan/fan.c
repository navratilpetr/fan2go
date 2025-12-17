#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "driver/ledc.h"
#include "driver/pcnt.h"
#include "driver/gpio.h"

#include "config.h"

/* ========= Local mapping ========= */
#define MAX_FANS             CONFIG_MAX_FANS
#define RPM_INTERVAL_MS      FAN_RPM_INTERVAL_MS
#define DETECT_INTERVAL_MS   FAN_DETECT_MS
#define DETECT_MIN_PULSES    FAN_DETECT_MIN_PULSES

#define FALLBACK_TIMEOUT_MS  FAN_FALLBACK_MS
#define FALLBACK_DUTY        FAN_FALLBACK_DUTY

#define CAL_STEP_PERCENT     FAN_CAL_STEP_PERCENT
#define CAL_SETTLE_MS        FAN_CAL_SETTLE_MS

#define RPM_TOLERANCE        50   /* tolerance in RPM for calibration */

#define LEDC_TIMER           LEDC_TIMER_0
#define LEDC_MODE_USED       LEDC_HIGH_SPEED_MODE
#define LEDC_FREQ_HZ         FAN_PWM_FREQ_HZ

#if FAN_PWM_RES_BITS == 8
#define LEDC_RESOLUTION      LEDC_TIMER_8_BIT
#else
#error "Unsupported FAN_PWM_RES_BITS (only 8 implemented)"
#endif

/* ========= Data structure ========= */
typedef struct {
    int index;
    gpio_num_t pwm_gpio;
    ledc_channel_t ledc_channel;
    gpio_num_t tach_gpio;
    pcnt_unit_t pcnt_unit;
    bool connected;
    uint32_t rpm;
    int duty_user;   /* 0–100 user */
    int duty_min;    /* calibrated minimum % */
    bool calibrated;
} fan_t;

/* ========= Prototypes ========= */
static void fan_ledc_init_global(void);
static void fan_ledc_init_channel(fan_t *fan);
static void fan_pcnt_init(fan_t *fan);
static void fan_detect_connected(fan_t fans[], int count);
static void fan_rpm_task(void *arg);
static void fallback_timer_cb(TimerHandle_t xTimer);
static void fan_calib_task(void *arg);

/* ========= Instances ========= */
static fan_t g_fans[MAX_FANS] = {
    { 0, FAN_PWM_0, LEDC_CHANNEL_0, FAN_TAC_0, PCNT_UNIT_0, false, 0, 0, 20, false },
    { 1, FAN_PWM_1, LEDC_CHANNEL_1, FAN_TAC_1, PCNT_UNIT_1, false, 0, 0, 20, false },
    { 2, FAN_PWM_2, LEDC_CHANNEL_2, FAN_TAC_2, PCNT_UNIT_2, false, 0, 0, 20, false },
    { 3, FAN_PWM_3, LEDC_CHANNEL_3, FAN_TAC_3, PCNT_UNIT_3, false, 0, 0, 20, false },
    { 4, FAN_PWM_4, LEDC_CHANNEL_4, FAN_TAC_4, PCNT_UNIT_4, false, 0, 0, 20, false },
};

/* ========= LEDC ========= */
static void fan_ledc_init_global(void)
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode      = LEDC_MODE_USED,
        .duty_resolution = LEDC_RESOLUTION,
        .timer_num       = LEDC_TIMER,
        .freq_hz         = LEDC_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);
}

static void fan_ledc_init_channel(fan_t *fan)
{
    ledc_channel_config_t ch = {
        .gpio_num   = fan->pwm_gpio,
        .speed_mode = LEDC_MODE_USED,
        .channel    = fan->ledc_channel,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0
    };
    ledc_channel_config(&ch);
}

/* ========= Helpers ========= */
void *fan_get_ptr(int idx)
{
    return (idx >= 0 && idx < MAX_FANS) ? &g_fans[idx] : NULL;
}

uint32_t fan_get_rpm(int idx)
{
    return (idx >= 0 && idx < MAX_FANS) ? g_fans[idx].rpm : 0;
}

bool fan_is_connected(int idx)
{
    return (idx >= 0 && idx < MAX_FANS) ? g_fans[idx].connected : false;
}

int fan_get_duty(int idx)
{
    return (idx >= 0 && idx < MAX_FANS) ? g_fans[idx].duty_user : 0;
}

int fan_get_min_start(int idx)
{
    return (idx >= 0 && idx < MAX_FANS) ? g_fans[idx].duty_min : 0;
}

/* ========= PWM Set ========= */
void fan_set_duty_percent(void *fan_ptr, int duty_percent)
{
    if (!fan_ptr) return;
    fan_t *fan = (fan_t *)fan_ptr;
    fan->duty_user = duty_percent;

    if (duty_percent < 0) duty_percent = 0;
    if (duty_percent > 100) duty_percent = 100;

    if (duty_percent == 0) {
        ledc_set_duty(LEDC_MODE_USED, fan->ledc_channel, 0);
        ledc_update_duty(LEDC_MODE_USED, fan->ledc_channel);
        return;
    }

    if (fan->calibrated) {
        int d = fan->duty_min + (duty_percent * (100 - fan->duty_min) / 100);
        duty_percent = d;
    }

    uint32_t max_duty = (1u << LEDC_RESOLUTION) - 1u;
    uint32_t duty = (max_duty * (uint32_t)duty_percent) / 100u;

    ledc_set_duty(LEDC_MODE_USED, fan->ledc_channel, duty);
    ledc_update_duty(LEDC_MODE_USED, fan->ledc_channel);
}

/* ========= PCNT / RPM ========= */
static void fan_pcnt_init(fan_t *fan)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << fan->tach_gpio),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    pcnt_config_t pcnt_config = {
        .pulse_gpio_num = fan->tach_gpio,
        .ctrl_gpio_num  = PCNT_PIN_NOT_USED,
        .pos_mode       = PCNT_COUNT_INC,
        .neg_mode       = PCNT_COUNT_DIS,
        .lctrl_mode     = PCNT_MODE_KEEP,
        .hctrl_mode     = PCNT_MODE_KEEP,
        .counter_h_lim  = 32767,
        .counter_l_lim  = 0,
        .unit           = fan->pcnt_unit,
        .channel        = PCNT_CHANNEL_0,
    };

    pcnt_unit_config(&pcnt_config);
    pcnt_set_filter_value(fan->pcnt_unit, 1000);
    pcnt_filter_enable(fan->pcnt_unit);
    pcnt_counter_pause(fan->pcnt_unit);
    pcnt_counter_clear(fan->pcnt_unit);
    pcnt_counter_resume(fan->pcnt_unit);
}

static void fan_detect_connected(fan_t fans[], int count)
{
    for (int i = 0; i < count; ++i) {
        fans[i].connected = false;
        pcnt_counter_pause(fans[i].pcnt_unit);
        pcnt_counter_clear(fans[i].pcnt_unit);
        pcnt_counter_resume(fans[i].pcnt_unit);
    }

    vTaskDelay(pdMS_TO_TICKS(DETECT_INTERVAL_MS));

    for (int i = 0; i < count; ++i) {
        int16_t cnt = 0;
        pcnt_get_counter_value(fans[i].pcnt_unit, &cnt);
        fans[i].connected = (cnt >= DETECT_MIN_PULSES);
        pcnt_counter_clear(fans[i].pcnt_unit);
    }
}

/* ========= RPM Update ========= */
static void fan_rpm_task(void *arg)
{
    vTaskDelay(pdMS_TO_TICKS(RPM_INTERVAL_MS));

    while (1) {
        for (int i = 0; i < MAX_FANS; ++i) {
            if (!g_fans[i].connected) {
                g_fans[i].rpm = 0;
                continue;
            }

            int16_t pulses = 0;
            pcnt_get_counter_value(g_fans[i].pcnt_unit, &pulses);
            float rps = (float)pulses / (float)FAN_PULSES_PER_REV;
            if (rps < 0) rps = 0;
            g_fans[i].rpm = (uint32_t)(rps * 60.0f);
            pcnt_counter_clear(g_fans[i].pcnt_unit);
        }

        vTaskDelay(pdMS_TO_TICKS(RPM_INTERVAL_MS));
    }
}

/* ========= Fallback Timer ========= */
static TimerHandle_t s_fallback_timer = NULL;

static void fallback_timer_cb(TimerHandle_t xTimer)
{
    for (int i = 0; i < MAX_FANS; ++i) {
        if (g_fans[i].connected)
            fan_set_duty_percent(&g_fans[i], FALLBACK_DUTY);
    }
}

void fan_backend_alive(void)
{
    if (s_fallback_timer)
        xTimerReset(s_fallback_timer, 0);
}

/* ========= Calibration ========= */
static void fan_calib_task(void *arg)
{
    vTaskDelay(pdMS_TO_TICKS(500));

    for (int i = 0; i < MAX_FANS; ++i) {
        if (!g_fans[i].connected) continue;

        fan_set_duty_percent(&g_fans[i], 0);
        vTaskDelay(pdMS_TO_TICKS(CAL_SETTLE_MS));

        uint32_t prev = fan_get_rpm(i);
        int found = 100;

        for (int d = 5; d <= 100; d += CAL_STEP_PERCENT) {
            fan_set_duty_percent(&g_fans[i], d);
            vTaskDelay(pdMS_TO_TICKS(CAL_SETTLE_MS));

            uint32_t now = fan_get_rpm(i);
            if (now > prev + RPM_TOLERANCE) {
                found = d;
                break;
            }
            prev = now;
        }

        if (found < 1) found = 1;
        g_fans[i].duty_min = found;
        g_fans[i].calibrated = true;

        fan_set_duty_percent(&g_fans[i], 0);

        printf("CALIB: fan%d min duty=%d%%\n", i, found);
    }

    vTaskDelete(NULL);
}

void fan_calibrate_all(void)
{
    xTaskCreate(fan_calib_task, "fan_calib", 4096, NULL, 4, NULL);
}

/* ========= INIT ========= */
void fan_init_all(void)
{
    fan_ledc_init_global();

    for (int i = 0; i < MAX_FANS; ++i) {
        fan_ledc_init_channel(&g_fans[i]);
        fan_pcnt_init(&g_fans[i]);
    }

    fan_detect_connected(g_fans, MAX_FANS);

    #if FAN_ENABLE_CALIBRATION
    	fan_calibrate_all();
    #else
	/* bez kalibrace: použij default */
    	for (int i = 0; i < MAX_FANS; ++i) {
        	g_fans[i].duty_min  = FAN_DEFAULT_DUTY;
        	g_fans[i].calibrated = false;
    	}
    #endif
    
    s_fallback_timer = xTimerCreate("fb", pdMS_TO_TICKS(FALLBACK_TIMEOUT_MS),
                                    pdFALSE, NULL, fallback_timer_cb);
    if (s_fallback_timer) xTimerStart(s_fallback_timer, 0);

    xTaskCreate(fan_rpm_task, "fan_rpm", 4096, NULL, 5, NULL);
}

