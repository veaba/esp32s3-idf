#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "key.h"

void app_main(void)
{
  led_init();
  key_init();

  uint8_t key;
  while (1)
  {
    key = key_scan(0); // 扫描按键状态
   
    switch (key)
    {
      case BOOT_PRES: // 按键按下
       {
        printf("Key Pressed!\n");
         LED_TOGGLE(); // 切换LED状态
        break;
       }
      default:
         break;
    }
    //  vTaskDelay(10); // 延时100ms，避免按键抖动
  }
}