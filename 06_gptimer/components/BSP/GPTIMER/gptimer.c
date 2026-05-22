#include "gptimer.h"

static bool IRAM_ATTR timer_group_isr_callback(void *arg)
{
    timg_config_t *user_data = (timg_config_t *)arg;
    user_data->timer_count_value = 0;
    user_data->timer_count_value = timer_group_get_counter_value_in_isr(user_data->timer_group, user_data->timer_idx); // 获取当前定时器计数值

    LED_TOGGLE(); // 切换LED状态

    if (!user_data->auto_reload) // 如果不自动重载，重新设置定时器计数值
    {
        user_data->alarm_value += user_data->timing_time;                                                         // 更新下次报警值
        timer_group_set_alarm_value_in_isr(user_data->timer_group, user_data->timer_idx, user_data->alarm_value); // 设置新的定时器报警值
    }
    return 1; // 返回true表示需要清除中断状态
}
void gptimer_init(timg_config_t *timg_config)
{
    uint32_t clk_src_hz = 0;
    timer_config_t timer_config = {0};

    ESP_ERROR_CHECK(esp_clk_tree_src_get_freq_hz(timg_config->src_clk, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &clk_src_hz)); // 获取定时器时钟源频率

    timer_config.alarm_en = TIMER_ALARM_EN;
    timer_config.auto_reload = timg_config->auto_reload;
    timer_config.clk_src = timg_config->src_clk;
    timer_config.counter_dir = GPTIMER_COUNT_UP;
    timer_config.counter_en = TIMER_PAUSE;               // 先暂停定时器，配置完成后再启动
    timer_config.divider = clk_src_hz / 1 * 1000 * 1000; // 将时钟频率转换为MHz，得到每微秒的计数值

    ESP_ERROR_CHECK(timer_init(timg_config->timer_group, timg_config->timer_idx, &timer_config));                       // 初始化定时器
    ESP_ERROR_CHECK(timer_set_counter_value(timg_config->timer_group, timg_config->timer_idx, 0));                      // 将定时器计数值清零
    ESP_ERROR_CHECK(timer_set_alarm_value(timg_config->timer_group, timg_config->timer_idx, timg_config->alarm_value)); // 设置定时器报警值

    ESP_ERROR_CHECK(timer_enable_intr(timg_config->timer_group, timg_config->timer_idx));                                         // 使能定时器中断
    ESP_ERROR_CHECK(timer_isr_callback_add(timg_config->timer_group, timg_config->timer_idx, timer_group_isr_callback, NULL, 0)); // 注册定时器中断回调函数

    ESP_ERROR_CHECK(timer_start(timg_config->timer_group, timg_config->timer_idx)); // 启动定时器
}