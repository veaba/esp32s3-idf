#include "key.h"

void key_init(void)
{
  gpio_config_t gpio_init_struct = {0};
  gpio_init_struct.intr_type = GPIO_INTR_DISABLE;
  gpio_init_struct.mode = GPIO_MODE_INPUT; // 输入模式
  gpio_init_struct.pull_up_en = GPIO_PULLUP_ENABLE; // 使能上拉 高电平
  gpio_init_struct.pull_down_en = GPIO_PULLDOWN_DISABLE; // 失能下拉
  gpio_init_struct.pin_bit_mask = 1ull << BOOT_GPIO_PIN; // GPIO0，设置引脚
  gpio_config(&gpio_init_struct);
}

uint8_t key_scan(uint8_t mode)
{
  uint8_t key_val =  0;
  static uint8_t key_boot = 1;

  if(mode)
  {
    key_boot = 1; // 读取按键状态
  }

  if(key_boot && (BOOT == 0))
  {
    vTaskDelay(10); // 消抖延时
    key_boot = 0;

    if(BOOT == 0)
    {
      key_val = BOOT_PRES; // 按键按下
    }
  } 
  else if(BOOT)
  {
    key_boot = 1; // 按键释放
  }

  return key_val;
}