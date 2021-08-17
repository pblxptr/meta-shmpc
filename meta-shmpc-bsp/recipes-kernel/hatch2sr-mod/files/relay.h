#pragma once

#include <linux/gpio.h>

typedef struct relay {
  struct gpio_desc* gpio;
  int gpio_id;
} relay_t;

int relay_init(relay_t* relay, struct gpio_desc* gpio);
void relay_deinit(relay_t*);
void relay_set_to_open(relay_t* relay);
void relay_set_to_close(relay_t* relay);
