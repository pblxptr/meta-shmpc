#include "engine.h"

#include <linux/kernel.h>
#include <linux/jiffies.h>

#define ENGINE_MIN_SPEED_PCT    (10)
#define ENGINE_MAX_SPEED_PCT    (35)
#define ENGINE_PWM_PERIOD_NS    (10000000)
#define ENGINE_PWM_INITIAL_DUTY (0)

/* Private function declarations */
static bool engine_is_slow_start_enabled(engine_t* engine);
static void engine_perform_slow_start(engine_t* engine);
static void engine_set_speed_pct(engine_t* engine, int speed);
static void engine_handle_timer(struct timer_list* timer);

/* Helper function declarations */
static int speed_pct_to_duty_ns(int speed_pct, int period_ns);

/* Public function defintitions */
int engine_init(engine_t* engine, struct pwm_device* pwm)
{
  engine->pwm = pwm;
  engine->state = ENGINE_STATE_IDLE;
  engine->slow_start = true;
  engine->max_speed_pct = ENGINE_MAX_SPEED_PCT;
  //TODO: Once device tree is fixed, add pwm->state.period instaed of ENIGNE_PWM_PERIOD_NS
  if (pwm_config(engine->pwm, speed_pct_to_duty_ns(0, ENGINE_PWM_PERIOD_NS), ENGINE_PWM_PERIOD_NS)) {
    return -1;
  }
  return 0;
}

void engine_deinit(engine_t* engine)
{
  engine_stop(engine);

  engine->pwm = NULL;
}

void engine_start(engine_t* engine)
{
  pr_info("%s\n", __FUNCTION__);

  if (engine_get_state(engine) == ENGINE_STATE_RUNNING) {
    return;
  }
  pwm_enable(engine->pwm);
  if (engine_is_slow_start_enabled(engine)) {
    engine_perform_slow_start(engine);
  } else {
    engine_set_speed_pct(engine, ENGINE_MAX_SPEED_PCT);
  }

  engine->state = ENGINE_STATE_RUNNING;
}

void engine_stop(engine_t* engine)
{
  pr_info("%s\n", __FUNCTION__);

  if (engine_get_state(engine) == ENGINE_STATE_IDLE) {
    return;
  }

  del_timer_sync(&engine->timer); //TODO: Is it ok to call it here? Do we need to check whether timer had been configured before?
  engine_set_speed_pct(engine, 0);
  pwm_disable(engine->pwm);

  engine->state = ENGINE_STATE_IDLE;
}

int engine_get_max_speed_pct(engine_t* engine)
{
  pr_info("%s\n", __FUNCTION__);

  return engine->max_speed_pct;
}

int engine_set_max_speed_pct(engine_t* engine, int value_pct)
{
  pr_info("%s\n", __FUNCTION__);

  if (value_pct < ENGINE_MIN_SPEED_PCT || value_pct > ENGINE_MAX_SPEED_PCT) {
    return -EINVAL;
  }
  else {
    engine->max_speed_pct = value_pct;
    return 0;
  }
}

engine_state_t engine_get_state(engine_t* engine)
{
  return engine->state;
}

void engine_set_slow_start(engine_t* engine, bool slow_start)
{
  engine->slow_start = slow_start;
}

bool engine_get_slow_start(engine_t* engine)
{
  return engine->slow_start;
}

/* Private function definitions */
bool engine_is_slow_start_enabled(engine_t* engine)
{
  return engine->slow_start == true;
}

void engine_perform_slow_start(engine_t* engine)
{
  pr_info("%s\n", __FUNCTION__);

  timer_setup(&engine->timer, engine_handle_timer, 0);
  engine_set_speed_pct(engine, ENGINE_MIN_SPEED_PCT);
  mod_timer(&engine->timer, jiffies + msecs_to_jiffies(100));
}

void engine_set_speed_pct(engine_t* engine, int speed)
{
  pr_info("%s, speed: %d\n", __FUNCTION__, speed);

  engine->speed = speed;
  pwm_config(engine->pwm, speed_pct_to_duty_ns(speed, engine->pwm->state.period), engine->pwm->state.period);
}

void engine_handle_timer(struct timer_list* timer)
{
  engine_t* engine;

  pr_info("%s\n", __FUNCTION__);

  engine = container_of_safe(timer, engine_t, timer); //TODO: Add check

  if (engine->speed >= engine->max_speed_pct) {
    return;
  }

  engine_set_speed_pct(engine, engine->speed + 1); //increase by 1%
  mod_timer(&engine->timer, jiffies + msecs_to_jiffies(150));
}

/* Helper function defintions */
int speed_pct_to_duty_ns(int speed_pct, int period_ns)
{
  return period_ns / 100 * speed_pct;
}