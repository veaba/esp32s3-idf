#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "exit.h"

void app_main(void)
{
  led_init();
  exit_init();

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(100)); // 延时100ms，避免按键抖动
  }
}