#include "relay.h"
#include <stdbool.h>

#define DEFAULT_STATE 0
#define ALLOW_DIRECTION_CHANGE false

int relay_init(relay_t* relay, struct gpio_desc* gpio)
{
  relay->gpio = gpio;
  relay->gpio_id = desc_to_gpio(relay->gpio);

  gpiod_direction_output(relay->gpio, DEFAULT_STATE);
  gpiod_export(relay->gpio, ALLOW_DIRECTION_CHANGE);

  return 0;
}

void relay_deinit(relay_t* relay)
{
  gpiod_set_value(relay->gpio, DEFAULT_STATE);
  gpiod_unexport(relay->gpio);

  relay->gpio = NULL;
}

void relay_set_to_open(relay_t* relay)
{
  gpiod_set_value(relay->gpio, 1);
}

void relay_set_to_close(relay_t* relay)
{
  gpiod_set_value(relay->gpio, 0);
}
