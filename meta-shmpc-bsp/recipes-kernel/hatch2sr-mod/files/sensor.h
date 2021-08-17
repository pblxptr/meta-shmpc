#pragma once

#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <stdbool.h>

typedef enum sensor_value {
  SENSOR_VALUE_LOW = 0,
  SENSOR_VALUE_HIGH = 1,
  SENSOR_VALUE_DEACTIVATED
} sensor_value_t;

typedef struct sensor {
  struct gpio_desc* gpio;
  int gpio_id;
  int irq;
  bool is_active;
  struct timer_list timer;
} sensor_t;

int sensor_init(sensor_t* sensor, struct gpio_desc* gpio, irq_handler_t irqhandler);
void sensor_deinit(sensor_t* sensor);
sensor_value_t sensor_get_value(sensor_t* sensor);
void sensor_deactivate(sensor_t* sensor, int deactivation_time_ms);