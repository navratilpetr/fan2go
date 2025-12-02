/*
 * espfan.c – 5× ventilátorů (fan0–fan4), 5× PWM (pwm1–pwm5)
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#define FAN_COUNT 5   /* fan0–fan4 = 5 kusů */

static struct platform_device *pdev;
static struct device *hwmon_dev;

static DEFINE_MUTEX(lock);

static int pwm_value[FAN_COUNT] = {0};
static int rpm_value[FAN_COUNT] = {0};

/* -------- PWM sysfs -------- */

static ssize_t pwm_show(struct device *dev,
                        struct device_attribute *attr, char *buf)
{
    int id = to_sensor_dev_attr(attr)->index;
    int v;

    mutex_lock(&lock);
    v = pwm_value[id];
    mutex_unlock(&lock);

    return sprintf(buf, "%d\n", v);
}

static ssize_t pwm_store(struct device *dev,
                         struct device_attribute *attr,
                         const char *buf, size_t count)
{
    int id = to_sensor_dev_attr(attr)->index;
    int val;

    if (kstrtoint(buf, 10, &val) || val < 0 || val > 255)
        return -EINVAL;

    mutex_lock(&lock);
    pwm_value[id] = val;
    mutex_unlock(&lock);

    return count;
}

/* -------- FAN input sysfs -------- */

static ssize_t fan_input_show(struct device *dev,
                              struct device_attribute *attr, char *buf)
{
    int id = to_sensor_dev_attr(attr)->index;
    int v;

    mutex_lock(&lock);
    v = rpm_value[id];
    mutex_unlock(&lock);

    return sprintf(buf, "%d\n", v);
}

static ssize_t fan_input_store(struct device *dev,
                               struct device_attribute *attr,
                               const char *buf, size_t count)
{
    int id = to_sensor_dev_attr(attr)->index;
    int val;

    if (kstrtoint(buf, 10, &val) || val < 0)
        return -EINVAL;

    mutex_lock(&lock);
    rpm_value[id] = val;
    mutex_unlock(&lock);

    return count;
}

/* -------- Atributy pro 5 ventilátorů -------- */

static SENSOR_DEVICE_ATTR(pwm1, 0644, pwm_show, pwm_store, 0);
static SENSOR_DEVICE_ATTR(pwm2, 0644, pwm_show, pwm_store, 1);
static SENSOR_DEVICE_ATTR(pwm3, 0644, pwm_show, pwm_store, 2);
static SENSOR_DEVICE_ATTR(pwm4, 0644, pwm_show, pwm_store, 3);
static SENSOR_DEVICE_ATTR(pwm5, 0644, pwm_show, pwm_store, 4);

static SENSOR_DEVICE_ATTR(fan1_input, 0644, fan_input_show, fan_input_store, 0);
static SENSOR_DEVICE_ATTR(fan2_input, 0644, fan_input_show, fan_input_store, 1);
static SENSOR_DEVICE_ATTR(fan3_input, 0644, fan_input_show, fan_input_store, 2);
static SENSOR_DEVICE_ATTR(fan4_input, 0644, fan_input_show, fan_input_store, 3);
static SENSOR_DEVICE_ATTR(fan5_input, 0644, fan_input_show, fan_input_store, 4);

static struct attribute *espfan_attrs[] = {
    &sensor_dev_attr_pwm1.dev_attr.attr,
    &sensor_dev_attr_pwm2.dev_attr.attr,
    &sensor_dev_attr_pwm3.dev_attr.attr,
    &sensor_dev_attr_pwm4.dev_attr.attr,
    &sensor_dev_attr_pwm5.dev_attr.attr,

    &sensor_dev_attr_fan1_input.dev_attr.attr,
    &sensor_dev_attr_fan2_input.dev_attr.attr,
    &sensor_dev_attr_fan3_input.dev_attr.attr,
    &sensor_dev_attr_fan4_input.dev_attr.attr,
    &sensor_dev_attr_fan5_input.dev_attr.attr,

    NULL
};

ATTRIBUTE_GROUPS(espfan);

/* -------- Init / Exit -------- */

static int __init espfan_init(void)
{
    pdev = platform_device_register_simple("espfan", -1, NULL, 0);
    if (IS_ERR(pdev))
        return PTR_ERR(pdev);

    hwmon_dev = hwmon_device_register_with_groups(&pdev->dev,
                                                  "espfan",
                                                  NULL,
                                                  espfan_groups);
    if (IS_ERR(hwmon_dev)) {
        platform_device_unregister(pdev);
        return PTR_ERR(hwmon_dev);
    }

    pr_info("espfan: hwmon registered (5 fans)\n");
    return 0;
}

static void __exit espfan_exit(void)
{
    hwmon_device_unregister(hwmon_dev);
    platform_device_unregister(pdev);
    pr_info("espfan: unloaded\n");
}

module_init(espfan_init);
module_exit(espfan_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("assistant");
MODULE_DESCRIPTION("ESP32 5-fan hwmon interface");

