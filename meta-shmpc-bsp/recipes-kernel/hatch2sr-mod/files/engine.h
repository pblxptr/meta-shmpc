#pragma once

#include <linux/pwm.h>
#include <linux/timer.h>
#include <stdbool.h>

typedef enum engine_state {
  ENGINE_STATE_IDLE,
  ENGINE_STATE_RUNNING
} engine_state_t;

typedef struct engine {
  struct pwm_device* pwm;
  engine_state_t state;
  bool slow_start;
  int max_speed_pct;
  int speed;
  struct timer_list timer;
} engine_t;

int engine_init(engine_t* enigne, struct pwm_device* pwm);
void engine_deinit(engine_t* engine);
void engine_start(engine_t* engine);
void engine_stop(engine_t* engine);
int  engine_get_max_speed_pct(engine_t* engine);
int engine_set_max_speed_pct(engine_t* engine, int value_pct);
engine_state_t engine_get_state(engine_t* engine);
void engine_set_slow_start(engine_t* engine, bool slow_start);
bool engine_get_slow_start(engine_t* engine);