#ifndef __LEDC_H_
#define __LEDC_H_

#include "driver/gpio.h"
#include "driver/ledc.h"

#define LEDC_PWN_TIMER LEDC_TIMER_0
#define LEDC_PWN_CH0_GPIO GPIO_NUM_1
#define LEDC_PWN_CH0_CHANNEL LEDC_CHANNEL_0

typedef struct
{
  ledc_clk_cfg_t clk_cfg;           // LEDC auto select the source clock.
  ledc_timer_t timer_num;           // LEDC timer index.
  uint32_t freq_hz;                 // LEDC frequency of PWM signal.
  ledc_timer_bit_t duty_resolution; // LEDC duty bit width.
  ledc_channel_t channel;           // LEDC channel index.
  uint32_t duty;                    // LEDC PWM duty cycle, the range is [0, (2**duty_resolution) - 1].
  int gpio_num;                     // LEDC output gpio_num, if you want to disable output, set this to -1.
} ledc_config_t;

uint32_t ledc_duty_pow(uint32_t duty, uint8_t duty_resolution);
void ledc_init(ledc_config_t *ledc_config);
void ledc_pwn_set_duty(ledc_config_t *ledc_config, uint32_t duty);
#endif
