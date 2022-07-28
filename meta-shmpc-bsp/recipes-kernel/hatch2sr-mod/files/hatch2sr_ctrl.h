#pragma once

#include <linux/gpio.h>
#include <linux/pwm.h>
#include <stdbool.h>

#include "engine.h"
#include "sensor.h"
#include "relay.h"

typedef enum hatch_status {
  HATCH_STATUS_OPEN,
  HATCH_STATUS_CLOSED,
  HATCH_STATUS_CHANGING_POSITION,
  HATCH_STATUS_FAULTY,
  HATCH_STATUS_UNDEFINED
} hatch_status;

typedef struct hatch2sr {
  engine_t engine;
  sensor_t openpos;
  sensor_t closedpos;
  struct relay relay;
} hatch2sr;

int hatch2sr_init(struct pwm_device* pwm, struct gpio_desc* openpos, struct  gpio_desc* closepos, struct gpio_desc* relay);
void hatch2sr_deinit(void);
hatch2sr* hatch2sr_get(void);
void hatch2sr_open(void);
void hatch2sr_close(void);
void hatch2sr_stop(void);
hatch_status hatch2sr_get_status(void);
void hatch2sr_engine_set_slow_start(bool slow_start);
bool hatch2sr_engine_get_slow_start(void);
int hatch2sr_engine_get_max_speed_pct(void);
int hatch2sr_engine_set_max_speed_pct(int max_speed_pct);