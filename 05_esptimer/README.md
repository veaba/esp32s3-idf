# 05_esptimer

> 宇宙为你闪烁

## 项目概述

**驱动方式**: 硬件定时器 (ESP Timer / Hardware Timer)

**核心功能**: 使用 ESP-IDF 的 `esp_timer` API 创建周期性定时器，每秒触发一次切换 LED 状态。

## 硬件连接

| 信号 | GPIO | 说明 |
|------|------|------|
| LED | GPIO1 | 连接到 LED (低电平点亮) |

## 软件架构

```
app_main()
  ├── led_init()                    // 初始化 LED GPIO1
  └── timer_init(1000000)          // 创建 1s 周期性定时器
       └── 定时器回调: timer_callback()
             └── LED_TOGGLE()       // 切换 LED

(主任务完成初始化后可删除或进入深度睡眠)
```

## 关键代码

### main.c

```c
void app_main(void)
{
    led_init();
    timer_init(1 * 1000 * 1000);  // 1s = 1000000 us
}
```

### esptimer.c

```c
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

### esptimer.h

```c
void timer_init(uint64_t period_us);
```

## esp_timer API 简介

| 函数 | 说明 |
|------|------|
| `esp_timer_create()` | 创建定时器实例 |
| `esp_timer_start_periodic()` | 启动周期性定时器 |
| `esp_timer_start_once()` | 启动单次定时器 |
| `esp_timer_stop()` | 停止定时器 |
| `esp_timer_delete()` | 删除定时器 |

### 定时器创建参数

```c
esp_timer_create_args_t timer_args = {
    .callback = timer_callback,    // 回调函数
    .arg = NULL,                    // 传递给回调的参数
    .name = "timer_name"           // 定时器名称 (用于调试)
};
```

## 定时器类型

| 类型 | 函数 | 用途 |
|------|------|------|
| 周期性 | `esp_timer_start_periodic()` | 固定间隔重复执行 |
| 单次 | `esp_timer_start_once()` | 执行一次后自动停止 |

## 性能特点

| 指标 | 值 |
|------|-----|
| 定时精度 | **微秒级** (硬件定时器) |
| CPU 占用 | **极低** (仅中断时占用) |
| 功耗 | **最低** (可配合深度睡眠) |
| 主任务状态 | 可删除或进入睡眠 |

## 与软件延时对比

| 特性 | vTaskDelay (软件) | esp_timer (硬件) |
|------|-------------------|------------------|
| 精度 | 毫秒级，受调度影响 | **微秒级** |
| CPU 占用 | 持续运行 | **仅中断时** |
| 功耗 | 高 | **低** |
| 实现复杂度 | 简单 | 中等 |
| 定时范围 | 受 tick rate 限制 | **us ~ h** |

## 与 01_led 轮询方式对比

| 特性 | 01_led (轮询) | 05_esptimer (硬件) |
|------|---------------|-------------------|
| 定时精度 | 受 FreeRTOS 调度影响 | **硬件级精度** |
| CPU 占用 | 持续运行 (100%) | **中断时短暂占用** |
| 功耗 | 高 | **低** |
| 主任务 | 必须保持运行 | **可删除/睡眠** |
| 代码复杂度 | 简单 | 中等 |

## 优缺点

**优点**:
- **最高精度**: 硬件定时器，不受 CPU 负载影响
- **最低功耗**: 定时器基于硬件中断，CPU 可深度睡眠
- **资源占用少**: 定时器硬件模块功耗极低
- **灵活配置**: 支持微秒级精度，周期可动态调整

**缺点**:
- 需要理解 esp_timer API
- 回调函数需注意执行时间
- 回调中不能使用阻塞调用

## 适用场景

- **精确定时**: 周期性采样、通信超时等
- **低功耗应用**: 电池供电设备的定期唤醒
- **多任务协调**: 定时触发任务执行
- **任何需要可靠定时的场景**

## 优化方向

### 1. 主任务进入深度睡眠

```c
void app_main(void)
{
    led_init();
    timer_init(1 * 1000 * 1000);
    esp_deep_sleep_start();  // 定时器可唤醒
}
```

### 2. 记录定时器触发次数

```c
static volatile int timer_count = 0;

void timer_callback(void *arg)
{
    timer_count++;
    LED_TOGGLE();
}
```

### 3. 动态修改定时周期

```c
esp_timer_handle_t timer_handle;

void change_period(uint64_t new_period_us)
{
    esp_timer_stop(timer_handle);
    esp_timer_start_periodic(timer_handle, new_period_us);
}
```

## 原理图

> file:///E:/%E7%A1%AC%E4%BB%B6%E7%B1%BB%E8%BD%AF%E4%BB%B6/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91DNESP32S3%E5%BC%80%E5%8F%91%E6%9D%BF/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91DNESP32S3%E5%BC%80%E5%8F%91%E6%9D%BF%E8%B5%84%E6%96%99%EF%BC%88A%E7%9B%98%EF%BC%89/3%EF%BC%8C%E5%8E%9F%E7%90%86%E5%9B%BE/ATK_DNESP32S3_V1.2.pdf

## 编译烧录

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```