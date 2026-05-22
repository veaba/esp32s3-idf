# 06_gptimer

> 宇宙为你闪烁

ESP32-S3 通用定时器示例，通过定时器中断控制 LED 状态切换。

## 功能描述

- 初始化 GPTIMER0，每秒触发一次定时器中断
- 中断发生时切换 LED 状态（GPIO1）
- 主循环打印定时器计数值

## 硬件配置

- **开发板**: ATK-DNESP32S3 V1.2
- **LED**: GPIO1
- **定时器**: TIMER_GROUP_0, TIMER_0
- **时钟源**: APB (80MHz)

## 项目结构

```
06_gptimer/
├── main/
│   └── main.c              # 应用入口
├── components/
│   └── BSP/
│       ├── LED/
│       │   ├── led.c       # LED 驱动
│       │   └── led.h
│       └── GPTIMER/
│           ├── gptimer.c   # 定时器驱动
│           └── gptimer.h
├── CMakeLists.txt
└── sdkconfig
```

## 驱动配置

| 参数 | 值 |
|------|-----|
| 时钟源 | APB (80MHz) |
| 分频系数 | 80 (1μs 计数1) |
| 定时周期 | 1,000,000 μs (1秒) |
| 自动重载 | 使能 |

## 原理图

> file:///E:/%E7%A1%AC%E4%BB%B6%E7%B1%BB%E8%BD%AF%E4%BB%B6/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91DNESP32S3%E5%BC%80%E5%8F%91%E6%9D%BF/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91DNESP32S3%E5%BC%80%E5%8F%91%E6%9D%BF%E8%B5%84%E6%96%99%EF%BC%88A%E7%9B%98%EF%BC%89/3%EF%BC%8C%E5%8E%9F%E7%90%86%E5%9B%BE/ATK_DNESP32S3_V1.2.pdf

## 构建与烧录

```bash
idf.py build
idf.py flash monitor
```

## 已知问题

1. **Divider 计算错误**: `gptimer.c:30` 中 `divider = clk_src_hz / 1 * 1000 * 1000` 会导致值超出范围（最大65535），应为 `divider = clk_src_hz / 1000000`
2. **类型定义不匹配**: `gptimer.h` 中 `timer_group` 和 `timer_idx` 应使用 `timer_group_t` 和 `timer_idx_t`