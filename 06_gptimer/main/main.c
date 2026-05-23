#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "gptimer.h"
#include "driver/timer.h"
#include "esp_log.h"

void app_main(void)
{
    timg_config_t *timgr_config = malloc(sizeof(timg_config_t));
    led_init();

    timgr_config->timer_count_value = 0;
    timgr_config->src_clk = TIMER_SRC_CLK_DEFAULT;         // 定时器时钟源选择APB
    timgr_config->timer_group = TIMER_GROUP_0;             // 定时器组选择
    timgr_config->timer_idx = TIMER_0;                     // 定时器索引选择
    timgr_config->timing_time = 1 * 1000 * 1000;           // 定时器计数时间，单位为微秒
    timgr_config->alarm_value = timgr_config->timing_time; // 定时器报警值设置为计数时间
    timgr_config->auto_reload = TIMER_AUTORELOAD_DIS;      // 定时器自动重载使能，关闭
    // timgr_config->auto_reload = TIMER_AUTORELOAD_EN;       // 定时器自动重载使能，开启

    gptimer_init(timgr_config); // 初始化GPTIMER

    while (1)
    {
        if (timgr_config->timer_count_value != 0) // 如果定时器计数值不为0，说明定时器中断回调函数已经被调用过
        {
            ESP_LOGI("gp timer", "Timer count value: %llu us", timgr_config->timer_count_value); // 打印定时器计数值
            timgr_config->timer_count_value = 0;                                                 // 将定时器计数值清零，等待下一次定时器中断回调函数调用
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // 主任务每隔10毫秒执行一次
    }
}
