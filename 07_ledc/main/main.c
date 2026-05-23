#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ledc.h"
#include <stdio.h>

void app_main(void)
{
    uint8_t dir = 1;
    uint16_t led_pwn_val = 0;

    ledc_config_t *ledc_config = malloc(sizeof(ledc_config_t));

    ledc_config->clk_cfg = LEDC_AUTO_CLK;
    ledc_config->timer_num = LEDC_TIMER_0;
    ledc_config->freq_hz = 1000;
    ledc_config->duty_resolution = LEDC_TIMER_14_BIT;
    ledc_config->gpio_num = LEDC_PWN_CH0_GPIO;
    ledc_config->channel = LEDC_CHANNEL_0;
    ledc_config->duty = 0;

    ledc_init(ledc_config);

    while (1)
    {

        vTaskDelay(pdMS_TO_TICKS(50));

        if (dir == 1)
        {
            led_pwn_val += 5;
        }
        else
        {
            led_pwn_val -= 5;
        }
        if (led_pwn_val == 100)
        {
            dir = 0;
        }
        if (led_pwn_val == 0)
        {
            dir = 1;
        }
        printf("led_pwn_val: %d\n", led_pwn_val);
        ledc_pwn_set_duty(ledc_config, led_pwn_val);
    }
}