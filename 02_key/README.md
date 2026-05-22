# 02_key

> 宇宙为你闪烁

## 项目概述

**驱动方式**: 轮询 (Polling) - 软件按键扫描

**核心功能**: 扫描按键状态，检测到按键按下时切换 LED 状态，使用软件消抖处理。

## 硬件连接

| 信号 | GPIO | 说明 |
|------|------|------|
| LED | GPIO1 | 连接到 LED (低电平点亮) |
| KEY | GPIO0 | BOOT 按键 (低电平按下，上拉) |

## 软件架构

```
app_main()
  ├── led_init()           // 初始化 LED GPIO1
  ├── key_init()           // 初始化 KEY GPIO0 (输入，上拉)
  └── while(1)
        ├── key = key_scan(0)   // 扫描按键
        └── if(key == BOOT_PRES)
              LED_TOGGLE()       // 切换 LED
```

## 关键代码

### main.c

```c
void app_main(void)
{
  led_init();
  key_init();

  uint8_t key;
  while (1)
  {
    key = key_scan(0);
    switch (key)
    {
      case BOOT_PRES:
        printf("Key Pressed!\n");
        LED_TOGGLE();
        break;
      default:
        break;
    }
  }
}
```

### key.c

```c
uint8_t key_scan(uint8_t mode)
{
  static uint8_t key_boot = 1;

  if (mode)
  {
    key_boot = 1;
  }

  if (key_boot && (BOOT == 0))
  {
    vTaskDelay(10);  // 消抖延时 10ms
    key_boot = 0;

    if (BOOT == 0)
    {
      key_val = BOOT_PRES;
    }
  }
  else if (BOOT)
  {
    key_boot = 1;
  }

  return key_val;
}
```

### key.h

```c
#define BOOT_GPIO_PIN GPIO_NUM_0
#define BOOT gpio_get_level(BOOT_GPIO_PIN)
#define BOOT_PRES 1

void key_init(void);
uint8_t key_scan(uint8_t mode);
```

## API 说明

| 函数 | 说明 |
|------|------|
| `led_init()` | 初始化 LED GPIO (GPIO1) |
| `key_init()` | 初始化按键 GPIO (GPIO0, 输入上拉) |
| `key_scan(mode)` | 扫描按键，mode=1 时重置状态，返回 BOOT_PRES 表示按下 |
| `LED_TOGGLE()` | 切换 LED 状态 |

## 消抖机制

软件消抖流程：
1. 检测到按键低电平 (`BOOT == 0`)
2. 延时 10ms 等待电平稳定
3. 再次确认按键仍为低电平
4. 返回按键按下事件
5. 等待按键释放后重置状态

## 性能特点

| 指标 | 值 |
|------|-----|
| CPU 占用 | 持续轮询 (高) |
| 功耗 | 较高 |
| 响应延迟 | 最多 10ms (消抖时间) |
| 按键检测 | 轮询方式，软件消抖 |

## 优缺点

**优点**:
- 实现简单直观
- 状态机逻辑清晰
- 可扩展多按键支持

**缺点**:
- CPU 持续轮询，功耗高
- 消抖时间固定 10ms，响应较慢
- 无法实现按键释放检测

## 与 03_exit 对比

| 特性 | 02_key (轮询) | 03_exit (中断) |
|------|---------------|----------------|
| 响应速度 | 最多 10ms | 即时 |
| CPU 占用 | 持续轮询 | 等待中断 |
| 功耗 | 高 | 低 |
| 消抖方式 | 软件延时 | 可在中断中处理 |

## 适用场景

- 学习按键扫描原理
- 对功耗无要求的场景
- 简单的用户交互

## 原理图

> file:///E:/%E7%A1%AC%E4%BB%B6%E7%B1%BB%E8%BD%AF%E4%BB%B6/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91DNESP32S3%E5%BC%80%E5%8F%91%E6%9D%BF/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91DNESP32S3%E5%BC%80%E5%8F%91%E6%9D%BF%E8%B5%84%E6%96%99%EF%BC%88A%E7%9B%98%EF%BC%89/3%EF%BC%8C%E5%8E%9F%E7%90%86%E5%9B%BE/ATK_DNESP32S3_V1.2.pdf

## 编译烧录

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```