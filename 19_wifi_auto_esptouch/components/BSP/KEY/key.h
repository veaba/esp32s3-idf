#ifndef __KEY_H_
#define __KEY_H_

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define BOOT_GPIO_PIN GPIO_NUM_0

#define BOOT gpio_get_level(BOOT_GPIO_PIN)

// BOOT 按键按下
#define BOOT_PRES 1

void key_init(void);
uint8_t key_scan(uint8_t mode);

#endif