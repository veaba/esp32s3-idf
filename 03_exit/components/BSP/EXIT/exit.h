#ifndef __EXIT_H_
#define __EXIT_H_

#include "driver/gpio.h"
#include "esp_system.h"
#include "led.h"

#define BOOT_INT_GPIO_PIN GPIO_NUM_0

#define BOOT gpio_get_level(BOOT_INT_GPIO_PIN);

void exit_init(void);
#endif