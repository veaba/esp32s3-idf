# ESP32-S3 项目性能对比分析

## 对比项目

| 项目        | 驱动方式                           | 描述                                   |
|-------------|------------------------------------|----------------------------------------|
| 01_led      | 轮询 (Polling)                     | LED 闪烁，通过 FreeRTOS vTaskDelay 延时 |
| 03_exit     | GPIO 外部中断 (External Interrupt) | 按键中断触发 LED 切换                  |
| 05_esptimer | 硬件定时器 (Hardware Timer)        | ESP Timer 周期性触发 LED 切换          |

---

## 核心代码对比

### 01_led - 轮询方式

```c
void app_main(void)
{
  led_init();
  while (1)
  {
    LED_TOGGLE();
    vTaskDelay(pdMS_TO_TICKS(200)); // 200 ms
  }
}
```

### 03_exit - GPIO 外部中断

```c
// main.c
void app_main(void)
{
  led_init();
  exit_init();
  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(100)); // 延时100ms，避免按键抖动
  }
}

// exit.c - ISR Handler
static void IRAM_ATTR exit_gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    if (gpio_num == BOOT_INT_GPIO_PIN)
    {
        LED_TOGGLE();
    }
}

void exit_init(void)
{
    gpio_config_t gpio_init_struct = {0};
    gpio_init_struct.intr_type = GPIO_INTR_NEGEDGE;
    gpio_init_struct.mode = GPIO_MODE_INPUT;
    gpio_init_struct.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_init_struct.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_init_struct.pin_bit_mask = 1ull << BOOT_INT_GPIO_PIN;
    gpio_config(&gpio_init_struct);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BOOT_INT_GPIO_PIN, exit_gpio_isr_handler, (void *)BOOT_INT_GPIO_PIN);
}
```

### 05_esptimer - 硬件定时器

```c
// main.c
void app_main(void)
{
    led_init();
    timer_init(1 * 1000 * 1000); // 1s
}

// esptimer.c
void timer_callback(void *arg)
{
    LED_TOGGLE();
}

void timer_init(uint64_t period_us)
{
    esp_timer_handle_t timer_handle;
    const esp_timer_create_args_t timer_args = {
        .callback = timer_callback,
        .arg = NULL,
        .name = "05_timer_task"
    };
    esp_timer_create(&timer_args, &timer_handle);
    esp_timer_start_periodic(timer_handle, period_us);
}
```

---

## 性能对比

| 指标           | 01_led         | 03_exit          | 05_esptimer        |
|----------------|----------------|------------------|--------------------|
| **功耗**       | 高             | 中低             | **最低**           |
| **CPU占用**    | 持续运行       | 等待中断         | 周期性短暂占用     |
| **定时精度**   | 低(受调度影响) | 不确定(依赖按键) | **高(硬件定时器)** |
| **响应延迟**   | 最多200ms      | 即时             | 由定时器周期决定   |
| **实现复杂度** | 简单           | 中等             | 中等               |
| **主任务状态** | 持续运行       | 可进入IDLE       | 可删除或深度睡眠   |

---

## 各方案优缺点

### 01_led (轮询方式)

**优点：**

- 代码简单，易于理解和调试
- 适合初学者学习RTOS任务调度

**缺点：**

- CPU 不断运行，功耗最高
- 定时精度受 FreeRTOS 调度影响，不精确
- 浪费CPU资源，大部分时间在空循环

**适用场景：**

- 学习/演示目的
- 对功耗无要求的简单场景

### 03_exit (GPIO外部中断)

**优点：**

- 响应速度快，按键按下即可触发
- CPU 可在空闲时进入低功耗状态
- 功耗低于轮询方式

**缺点：**

- 需要处理按键消抖（当前有100ms消抖延时）
- 中断上下文执行时间尽量短，当前仅做LED toggle
- 多个中断源时需要管理优先级

**适用场景：**

- 外部事件驱动场景
- 用户交互（按键、传感器触发等）
- 需要即时响应外部信号

### 05_esptimer (硬件定时器)

**优点：**

- **最高精度**：硬件定时器不受CPU负载影响
- **最低功耗**：主任务可进入深度睡眠，仅定时器中断唤醒
- 定时周期可灵活配置（微秒级精度）
- 适合周期性任务

**缺点：**

- 需要配置 ESP Timer API
- 定时器回调需注意执行时间，避免过长阻塞

**适用场景：**

- 周期性采样/上报
- 精确延时需求
- 低功耗定期唤醒
- 任何需要精确定时的场景

---

## 功耗优化建议

### 01_led 优化方向

1. **改用硬件定时器驱动** - 替代软件延时
2. **使用 vTaskDelayUntil()** - 减少任务切换开销
3. **添加空闲任务** - 让 CPU 空闲时可进入低功耗

```c
// 优化示例：使用定时器替代轮询
void timer_callback(void *arg)
{
    LED_TOGGLE();
}
```

### 03_exit 优化方向

1. **缩短消抖时间** - 当前100ms可缩短到20-50ms
2. **使用状态机消抖** - 避免固定延时阻塞
3. **启用外部中断唤醒深度睡眠** - 进一步降低功耗

```c
// 优化示例：消抖状态机
typedef enum {
    BTN_IDLE,
    BTN_PRESSED,
    BTN_DEBOUNCING
} btn_state_t;
```

### 05_esptimer 优化方向

1. **主任务进入深度睡眠** - 定时器可唤醒
2. **使用One-shot定时器** - 替代periodic，减少中断次数
3. **批量处理** - 将多次操作合并到一次定时回调

```c
// 优化示例：深度睡眠 + 定时器唤醒
esp_sleep_enable_timer_wakeup(1 * 1000 * 1000); // 1s
esp_deep_sleep_start();
```

---

## 总结

| 场景推荐        | 推荐方案        |
|-----------------|-----------------|
| 学习入门        | 01_led          |
| 外部事件触发    | 03_exit         |
| 精确定时/低功耗 | **05_esptimer** |
| 综合最佳        | **05_esptimer** |

对于大多数嵌入式应用，**05_esptimer (硬件定时器)** 是最佳选择：

- 精确的定时控制
- 最低的功耗消耗
- 释放CPU去做其他任务
