#ifndef __GPTIMER_H_
#define __GPTIMER_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "driver/timer.h"
#include "esp_clk_tree.h"

typedef struct
{
  timer_src_clk_t src_clk;        // 定时器时钟源
  int timer_group;                // 定时器组
  int timer_idx;                  // 定时器索引
  uint64_t timing_time;           // 定时器计时时间
  uint64_t alarm_value;           // 定时器报警值
  timer_autoreload_t auto_reload; // 定时器自动重载
  uint64_t timer_count_value;     // 定时器间隔时间（秒）
} timg_config_t;

void gptimer_init(timg_config_t *config);
#endif