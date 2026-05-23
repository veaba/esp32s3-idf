#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "led.h"
#include "iic.h"
#include "xl9555.h"
#include <stdio.h>

i2c_obj_t i2c0_master;

void display_info(void)
{

  printf("\n");
  printf(" ******************************** \n");
  printf("GBC-ESP32 Mini\n");
}

void app_main(void)
{

  uint8_t key;
  i2c0_master = iic_init(I2C_NUM_0); // 初始化 I2C0
  xl9555_init(i2c0_master);          // 初始化 XL9555
  display_info();

  while (1)
  {
    key = xl9555_key_scan(0); // 扫描按键状态
    switch (key)
    {
    case KEY0_PRES:
    {
      printf("KEY0 Pressed\n");
      xl9555_pin_write(BEEP_IO, 0); // 打开蜂鸣器
      break;
    }

    case KEY1_PRES:
    {
      printf("KEY1 Pressed\n");
      xl9555_pin_write(BEEP_IO, 1); // 关闭蜂鸣器
      break;
    }

    case KEY2_PRES:
    {
      printf("KEY2 Pressed\n");
      LED(0);
      break;
    }
    case KEY3_PRES:
    {
      printf("KEY3 Pressed\n");
      LED(1);
      break;
    }
    default:
    {
      break;
    }
    }
    vTaskDelay(pdMS_TO_TICKS(200)); // 延时 100ms
  }
}
