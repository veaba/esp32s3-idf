#include "esptimer.h"

void timer_callback(void *arg)
{
    LED_TOGGLE(); // 切换LED状态
}

void timer_init(uint64_t period_us)
{
    esp_timer_handle_t timer_handle;

    const esp_timer_create_args_t timer_args = {
        .callback = timer_callback, // 定时器回调函数
        .arg = NULL,                // 传递给回调函数的参数
        .name = "05_timer_task"     // 定时器名称
    };

    esp_timer_create(&timer_args, &timer_handle); // 创建定时器

    esp_timer_start_periodic(timer_handle, period_us); // 启动定时器，周期为指定微秒数
}
