#include "hatch2sr_ctrl.h"

#include <stddef.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/printk.h>
#include <linux/delay.h>
#include <stdbool.h>

#include "engine.h"
#include "relay.h"
#include "sensor.h"

#define EN_DEBOUNCE
#define SENSOR_DEACTIVATION_MS 3000

#ifdef EN_DEBOUNCE
#include <linux/jiffies.h>
extern unsigned long volatile jiffies;
#endif

/* Hatch private functions declarations */
static irqreturn_t openpos_sensor_isr(int irq, void* dev_id);
static irqreturn_t closedpos_sensor_isr(int irq, void* dev_id);
static irqreturn_t handle_sensor_isr(unsigned long* old_jiffies, sensor_t* sensor);
static bool can_change_position(void);
static bool is_changing_position(void);
static bool is_faulty(void);

static hatch2sr hatch;

int hatch2sr_init(struct pwm_device* pwm, struct gpio_desc* openpos, struct  gpio_desc* closedpos, struct gpio_desc* relay)
{
  if (pwm == NULL || openpos == NULL || closedpos == NULL || relay == NULL) {
    return -1;
  }

  //Initialize engine
  if (engine_init(&hatch.engine, pwm)) {
    return -1;
  }

  //Initialize sensors
  if (sensor_init(&hatch.openpos, openpos, openpos_sensor_isr)) {
    return -1;
  }

  if (sensor_init(&hatch.closedpos, closedpos, closedpos_sensor_isr)) {
    return -1;
  }

  // Relay init
  if (relay_init(&hatch.relay, relay)) {
    return -1;
  }

  return 0;
}

void hatch2sr_deinit()
{
  engine_deinit(&hatch.engine);
  sensor_deinit(&hatch.openpos);
  sensor_deinit(&hatch.closedpos);
  relay_deinit(&hatch.relay);
}

hatch2sr* hatch2sr_get()
{
  return &hatch;
}

void hatch2sr_open()
{
  pr_info("%s\n", __FUNCTION__);

  if (sensor_get_value(&hatch.openpos)) {
    return;
  }

  if (!can_change_position()) {
    return;
  }

  relay_set_to_open(&hatch.relay);
  mdelay(300); //TODO: Consider changing mdelay to flag, and run engine in workqueu
  sensor_deactivate(&hatch.closedpos, SENSOR_DEACTIVATION_MS);
  engine_start(&hatch.engine);
}

void hatch2sr_close()
{
  pr_info("%s\n", __FUNCTION__);

  if (sensor_get_value(&hatch.closedpos)) {
    return;
  }

  if (!can_change_position()) {
    return;
  }

  relay_set_to_close(&hatch.relay);
  mdelay(300); //TODO: Consider changing mdelay to flag, and run engine in workqueu
  sensor_deactivate(&hatch.openpos, SENSOR_DEACTIVATION_MS);
  engine_start(&hatch.engine);
}

void hatch2sr_stop()
{
  pr_info("%s\n", __FUNCTION__);

  engine_stop(&hatch.engine);
}

hatch_status hatch2sr_get_status()
{
  int open_sensor_val;
  int closed_sensor_val;
  engine_state_t engine_s;

  open_sensor_val = sensor_get_value(&hatch.openpos);
  closed_sensor_val = sensor_get_value(&hatch.closedpos);
  engine_s = engine_get_state(&hatch.engine);

  if (is_faulty()) {
    return HATCH_STATUS_FAULTY;
  }
  if (is_changing_position()) {
    return HATCH_STATUS_CHANGING_POSITION;
  }

  if (open_sensor_val) {
    return HATCH_STATUS_OPEN;
  } else if (closed_sensor_val) {
    return HATCH_STATUS_CLOSED;
  } else {
    return HATCH_STATUS_UNDEFINED;
  }
}

void hatch2sr_engine_set_slow_start(bool slow_start)
{
  engine_set_slow_start(&hatch.engine, slow_start);
}

bool hatch2sr_engine_get_slow_start(void)
{
  return engine_get_slow_start(&hatch.engine);
}

int hatch2sr_engine_get_max_speed_pct(void)
{
  return engine_get_max_speed_pct(&hatch.engine);
}

int hatch2sr_engine_set_max_speed_pct(int max_speed_pct)
{
  return engine_set_max_speed_pct(&hatch.engine, max_speed_pct);
}

/* Hatch private function declarations */
irqreturn_t openpos_sensor_isr(int irq, void* dev_id)
{
  static unsigned long old_jiffies = 0;

  pr_info("Open position sensor isr: %d\n", irq);

  return handle_sensor_isr(&old_jiffies, &hatch.openpos);
}

irqreturn_t closedpos_sensor_isr(int irq, void* dev_id)
{
  static unsigned long old_jiffies = 0;

  pr_info("Closed position sensor isr: %d\n", irq);

  return handle_sensor_isr(&old_jiffies, &hatch.closedpos);
}

irqreturn_t handle_sensor_isr(unsigned long* old_jiffies, sensor_t* sensor)
{
  #ifdef EN_DEBOUNCE
  unsigned long diff = jiffies - *old_jiffies;
  if (diff < 100)
  {
    return IRQ_HANDLED;
  }

  *old_jiffies = jiffies;
  #endif

  if (sensor_get_value(sensor) == SENSOR_VALUE_DEACTIVATED) {
    pr_info("Sensor deactivated skip isr\n");
    return IRQ_HANDLED;
  }

  engine_stop(&hatch.engine);

  return IRQ_HANDLED;
}

bool can_change_position(void)
{
  hatch_status status;

  status = hatch2sr_get_status();

  return status != HATCH_STATUS_CHANGING_POSITION
    && status != HATCH_STATUS_FAULTY
    && status != HATCH_STATUS_UNDEFINED;
}

bool is_changing_position(void)
{
  return engine_get_state(&hatch.engine) == ENGINE_STATE_RUNNING;
}

bool is_faulty(void)
{
  int open_sensor_val;
  int closed_sensor_val;

  open_sensor_val = sensor_get_value(&hatch.openpos);
  closed_sensor_val = sensor_get_value(&hatch.closedpos);

  return open_sensor_val && closed_sensor_val;
}