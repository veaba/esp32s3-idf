#include "led.h"

#include "driver/gpio.h"

void led_init()
{
  gpio_config_t gpio_init_struct = {0};
  gpio_init_struct.intr_type = GPIO_INTR_DISABLE;
  gpio_init_struct.mode = GPIO_MODE_INPUT_OUTPUT;        // 输入输出模式
  gpio_init_struct.pull_up_en = GPIO_PULLUP_ENABLE;      // 使能上拉 高电平
  gpio_init_struct.pull_down_en = GPIO_PULLDOWN_DISABLE; // 失能下拉
  gpio_init_struct.pin_bit_mask = 1ull << LED_GPIO_PIN;  // GPIO1，设置引脚
  gpio_config(&gpio_init_struct);

  LED(1); // 1 熄灭状态，调用宏
}