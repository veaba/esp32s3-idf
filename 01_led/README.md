# 01_led

> 宇宙为你闪烁

## 项目概述

**驱动方式**: 轮询 (Polling) - FreeRTOS 任务延时

**核心功能**: LED 周期性闪烁，通过 `vTaskDelay()` 实现 200ms 周期的状态切换。

## 硬件连接

| 信号 | GPIO | 说明 |
|------|------|------|
| LED | GPIO1 | 连接到 LED (低电平点亮) |

## 软件架构

```
app_main()
  └── led_init()          // 初始化 GPIO1 为输出
  └── while(1)
        └── LED_TOGGLE()  // 切换 LED 状态
        └── vTaskDelay(200ms)  // 延时 200ms
```

## 关键代码

### main.c

```c
void app_main(void)
{
  led_init();
  while (1)
  {
    LED_TOGGLE();
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}
```

### led.h

```c
#define LED_GPIO_PIN GPIO_NUM_1

#define LED(X) \
  do { \
    X ? gpio_set_level(LED_GPIO_PIN, PIN_SET) : gpio_set_level(LED_GPIO_PIN, PIN_RESET); \
  } while (0)

#define LED_TOGGLE() \
  do { \
    gpio_set_level(LED_GPIO_PIN, !gpio_get_level(LED_GPIO_PIN)); \
  } while (0)
```

## API 说明

| 函数 | 说明 |
|------|------|
| `led_init()` | 初始化 LED GPIO (GPIO1, 输出模式) |
| `LED_TOGGLE()` | 切换 LED 状态 |
| `LED(on)` | 设置 LED 状态 (1=熄灭, 0=点亮) |

## 性能特点

| 指标 | 值 |
|------|-----|
| 闪烁频率 | ~2.5 Hz (200ms 周期) |
| CPU 占用 | 持续运行 (高) |
| 功耗 | 较高 |
| 定时精度 | 受 FreeRTOS 调度影响 |

## 优缺点

**优点**:
- 代码简单，易于理解
- 适合 FreeRTOS 入门学习

**缺点**:
- CPU 不断空转，功耗高
- 定时精度受任务调度影响
- 浪费 CPU 资源

## 适用场景

- 学习/演示目的
- 对功耗无要求的简单场景

## 原理图

> file:///E:/%E7%A1%AC%E4%BB%B6%E7%B1%BB%E8%BD%AF%E4%BB%B6/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91DNESP32S3%E5%BC%80%E5%8F%91%E6%9D%BF/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91DNESP32S3%E5%BC%80%E5%8F%91%E6%9D%BF%E8%B5%84%E6%96%99%EF%BC%88A%E7%9B%98%EF%BC%89/3%EF%BC%8C%E5%8E%9F%E7%90%86%E5%9B%BE/ATK_DNESP32S3_V1.2.pdf

## 编译烧录

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```