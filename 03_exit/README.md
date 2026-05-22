# 03_exit

> 宇宙为你闪烁

## 项目概述

**驱动方式**: GPIO 外部中断 (External Interrupt)

**核心功能**: 配置 GPIO0 为外部中断源，下降沿触发，在中断服务程序 (ISR) 中切换 LED 状态。

## 硬件连接

| 信号 | GPIO | 说明 |
|------|------|------|
| LED | GPIO1 | 连接到 LED (低电平点亮) |
| BOOT Key | GPIO0 | BOOT 按键 (低电平按下，上拉) |

## 软件架构

```
app_main()
  ├── led_init()           // 初始化 LED GPIO1
  ├── exit_init()          // 配置 GPIO0 外部中断
  └── while(1)
        vTaskDelay(100ms)  // 主任务可进入低功耗等待

中断触发:
  exit_gpio_isr_handler()
    └── LED_TOGGLE()       // ISR 中切换 LED
```

## 关键代码

### main.c

```c
void app_main(void)
{
  led_init();
  exit_init();

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
```

### exit.c

```c
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
    gpio_init_struct.intr_type = GPIO_INTR_NEGEDGE;  // 下降沿触发
    gpio_init_struct.mode = GPIO_MODE_INPUT;
    gpio_init_struct.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_init_struct.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_init_struct.pin_bit_mask = 1ull << BOOT_INT_GPIO_PIN;

    gpio_config(&gpio_init_struct);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BOOT_INT_GPIO_PIN, exit_gpio_isr_handler,
                         (void *)BOOT_INT_GPIO_PIN);
}
```

### exit.h

```c
#define BOOT_INT_GPIO_PIN GPIO_NUM_0

void exit_init(void);
```

## 中断配置说明

| 配置项 | 值 | 说明 |
|--------|-----|------|
| 中断类型 | GPIO_INTR_NEGEDGE | 下降沿触发 |
| 模式 | GPIO_MODE_INPUT | 输入模式 |
| 上拉 | GPIO_PULLUP_ENABLE | 使能上拉电阻 |
| 下拉 | GPIO_PULLDOWN_DISABLE | 禁能下拉电阻 |

## API 说明

| 函数 | 说明 |
|------|------|
| `led_init()` | 初始化 LED GPIO (GPIO1) |
| `exit_init()` | 配置 GPIO0 外部中断 |
| `LED_TOGGLE()` | 切换 LED 状态 (可在 ISR 中调用) |

## ISR 编写要点

1. **IRAM_ATTR**: 中断处理函数必须放在 IRAM 中确保快速执行
2. **精简逻辑**: ISR 应尽量简短，只做必要操作
3. **避免阻塞**: 不能在 ISR 中使用 vTaskDelay 等阻塞调用
4. **线程安全**: 共享资源需注意竞态条件

## 性能特点

| 指标 | 值 |
|------|-----|
| 响应延迟 | **即时** (微秒级) |
| CPU 占用 | 低 (中断等待) |
| 功耗 | **低** |
| 定时精度 | 硬件级别，不受软件影响 |

## 与 02_key 对比

| 特性 | 02_key (轮询) | 03_exit (中断) |
|------|---------------|----------------|
| 响应速度 | 最多 10ms | **即时** (us级) |
| CPU 占用 | 持续轮询 | **等待中断** |
| 功耗 | 高 | **低** |
| 实现复杂度 | 简单 | 中等 |
| 消抖方式 | 软件延时 | **硬件边沿触发** |

## 优缺点

**优点**:
- 响应速度快，即时触发
- CPU 可在空闲时进入低功耗
- 功耗远低于轮询方式
- 中断机制更符合嵌入式开发模式

**缺点**:
- 中断服务程序需谨慎编写
- 多中断源时需管理优先级
- 调试相对复杂

## 适用场景

- **外部事件驱动**: 按键、传感器信号等
- **即时响应需求**: 紧急停车、限位检测等
- **低功耗应用**: 电池供电设备
- **多任务系统**: 中断与任务解耦

## 原理图

> file:///E:/%E7%A1%AC%E4%BB%B6%E7%B1%BB%E8%BD%AF%E4%BB%B6/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91DNESP32S3%E5%BC%80%E5%8F%91%E6%9D%BF/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91DNESP32S3%E5%BC%80%E5%8F%91%E6%9D%BF%E8%B5%84%E6%96%99%EF%BC%88A%E7%9B%98%EF%BC%89/3%EF%BC%8C%E5%8E%9F%E7%90%86%E5%9B%BE/ATK_DNESP32S3_V1.2.pdf

## 编译烧录

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```