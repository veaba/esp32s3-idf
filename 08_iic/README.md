# 08_iic

> 宇宙为你闪烁

ESP32-S3 I2C 总线示例，通过 I2C 控制 XL9555 IO 扩展芯片。

## 功能描述

- 初始化 ESP32-S3 I2C0 主机
- 驱动 XL9555 16 位 IO 扩展芯片
- 按键扫描（KEY0-KEY3）
- 控制蜂鸣器(BEEP)、扬声器(SPK_EN)
- LED 状态指示

## 硬件配置

- **开发板**: ATK-DNESP32S3 V1.2
- **I2C0 SDA**: GPIO41
- **I2C0 SCL**: GPIO42
- **XL9555 I2C 地址**: 0x20
- **中断引脚**: GPIO40

## 项目结构

```
08_iic/
├── main/
│   └── main.c              # 应用入口，按键扫描与控制
├── components/
│   └── BSP/
│       ├── IIC/
│       │   ├── iic.c       # I2C 主机驱动
│       │   └── iic.h       # I2C 头文件
│       ├── XL9555/
│       │   ├── xl9555.c    # IO 扩展芯片驱动
│       │   └── xl9555.h    # XL9555 头文件
│       ├── LED/
│       │   ├── led.c       # LED 驱动
│       │   └── led.h       # LED 头文件
│       └── CMakeLists.txt
├── CMakeLists.txt
├── sdkconfig
└── partitions-16MiB.csv
```

## IO 定义

| IO          | 功能         |
|-------------|--------------|
| AP_INT_IO   | AP 中断      |
| QMA_INT_IO  | QMA 中断     |
| SPK_EN_IO   | 扬声器使能   |
| BEEP_IO     | 蜂鸣器       |
| OV_PWDN_IO  | OV 掉电      |
| OV_RESET_IO | OV 复位      |
| GBC_LED_IO  | GBC LED      |
| GBC_KEY_IO  | GBC 按键     |
| LCD_BL_IO   | LCD 背光     |
| CT_RST_IO   | CT 复位      |
| SLCD_RST_IO | SLCD 复位    |
| SLCD_PWR_IO | SLCD 电源    |
| KEY3-0_IO   | 按键 3-0     |

## 构建与烧录

```bash
idf.py build
idf.py flash monitor
```