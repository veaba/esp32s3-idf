# 08_iic

> 宇宙为你闪烁

ESP32-S3 LEDC (LED Controller) PWM 示例，通过 PWM 控制 LED 亮度渐变。

## 功能描述

- 初始化 LEDC PWM 通道，使用 14 位分辨率
- 主循环逐步改变 PWM 占空比，实现 LED 呼吸灯效果
- 通过串口打印当前 PWM 值

## 硬件配置

- **开发板**: ATK-DNESP32S3 V1.2
- **LED**: GPIO1
- **PWM 通道**: LEDC_CHANNEL_0
- **定时器**: LEDC_TIMER_0

## 项目结构

```shell
07_ledc/
├── main/
│   └── main.c              # 应用入口，PWM 渐变逻辑
├── components/
│   └── BSP/
│       └── LEDC/
│           ├── ledc.c     # LEDC 驱动实现
│           └── ledc.h     # LEDC 驱动头文件
├── CMakeLists.txt
└── sdkconfig
```

## 驱动配置

| 参数       | 值               |
|------------|------------------|
| 时钟源     | LEDC_AUTO_CLK    |
| 频率       | 1000 Hz          |
| 分辨率     | 14 位            |
| 最大占空比 | 16383 (2^14 - 1) |
| GPIO       | GPIO1            |

## 原理图

> file:///E:/%E7%A1%AC%E4%BB%B6%E7%B1%BB%E8%BD%AF%E4%BB%B6/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91DNESP32S3%E5%BC%80%E5%8F%91%E6%9D%BF/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91DNESP32S3%E5%BC%80%E5%8F%91%E6%9D%BF%E8%B5%84%E6%96%99%EF%BC%88A%E7%9B%98%EF%BC%89/3%EF%BC%8C%E5%8E%9F%E7%90%86%E5%9B%BE/ATK_DNESP32S3_V1.2.pdf

## 构建与烧录

```bash
idf.py build
idf.py flash monitor
```

## 已知问题

1. **ledc_duty_pow 计算错误**: `ledc.c:3-11` 中 `(result * duty) / 100` 会在 duty=100 时超出范围（16384 > 16383），导致 LED 爆闪

```diff
- uint32_t ledc_duty_pow(uint32_t duty, uint8_t m, uint8_t n)
uint32_t ledc_duty_pow(uint32_t duty, uint8_t duty_resolution)
{
-   uint32_t result = 1;
- 
-   while (n--)
-   {
-     result *= m;
-   }
-   return (result * duty) / 100;
+ return (duty * ((1 << duty_resolution) - 1)) / 100;
}
```
