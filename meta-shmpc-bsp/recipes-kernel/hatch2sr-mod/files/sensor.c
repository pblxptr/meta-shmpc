#include "sensor.h"

#include <stdbool.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/printk.h>
#include <stddef.h>

/* Private function declarations */
static void sensor_handle_timer(struct timer_list* timer);

/* Public function implementations */
int sensor_init(sensor_t* sensor, struct gpio_desc* gpio, irq_handler_t irqhandler)
{
  sensor->gpio = gpio;
  sensor->gpio_id = desc_to_gpio(sensor->gpio);
  sensor->irq = gpio_to_irq(sensor->gpio_id);
  sensor->is_active = true;

  gpiod_direction_input(sensor->gpio);
  gpiod_export(sensor->gpio, false);

  if (request_irq(sensor->irq, irqhandler, IRQF_TRIGGER_FALLING,
      "hatch2sr", NULL))
  {
    pr_err("Cannot register IRQ: %d for sensor.\n", sensor->irq);
    return -EIO;
  }

  timer_setup(&sensor->timer, sensor_handle_timer, 0);

  return 0;
}

void sensor_deinit(sensor_t* sensor)
{
  free_irq(sensor->irq, NULL);
  gpiod_unexport(sensor->gpio);
  del_timer_sync(&sensor->timer);

  sensor->gpio = NULL;
}

sensor_value_t sensor_get_value(sensor_t* sensor)
{
  int value;

  if (!sensor->is_active) {
    return SENSOR_VALUE_DEACTIVATED;
  }

  value = gpiod_get_value(sensor->gpio);

  if (value) {
    return SENSOR_VALUE_HIGH;
  } else{
    return SENSOR_VALUE_LOW;
  }
}

void sensor_deactivate(sensor_t* sensor, int deactivation_time_ms)
{
  pr_info("%s\n", __FUNCTION__);

  mod_timer(&sensor->timer, jiffies + msecs_to_jiffies(deactivation_time_ms));

  sensor->is_active = false;
}

/* Private function implementations */
void sensor_handle_timer(struct timer_list* timer)
{
  sensor_t* sensor = NULL;

  pr_info("%s\n", __FUNCTION__);

  sensor = container_of_safe(timer, sensor_t, timer);

  if (sensor == NULL) {
    pr_err("Cannot handle sensor timer interrupt as the sensor is null\n");
    return;
  }

  sensor->is_active = true;
}