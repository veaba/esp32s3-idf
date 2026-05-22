#ifndef __ESPTIMER_H_
#define __ESPTIMER_H_

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "led.h"
#include "esp_timer.h"

void timer_init(uint64_t period_us);
#endif