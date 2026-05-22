#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "esptimer.h"

void app_main(void)
{
    led_init();
    timer_init(1 * 1000 * 1000); // 1s

    // while (1)
    // {
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    // }
}
