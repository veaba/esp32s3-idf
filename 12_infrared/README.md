# 12_infrared

> 宇宙为你闪烁

ESP32-S3 红外遥控解码示例，通过 RMT 外设接收 NEC 协议红外信号，并在 SPI LCD 上显示按键值。

## 功能描述

- 初始化 ESP32-S3 RMT 外设接收红外信号
- NEC 协议解码（支持重复帧检测）
- 驱动 SPI LCD 显示按键信息
- 通过 I2C0 驱动 XL9555 IO 扩展芯片
- LED 状态指示

## 硬件配置

- **开发板**: ATK-DNESP32S3 V1.2
- **RMT RX**: GPIO46
- **SPI LCD**: PISO 接口
- **I2C0 SDA**: GPIO41
- **I2C0 SCL**: GPIO42

## 项目结构

```
12_infrared/
├── main/
│   └── main.c              # 应用入口，红外解码与 LCD 显示
├── components/
│   └── BSP/
│       ├── LCD/
│       │   ├── lcd.c       # SPI LCD 驱动
│       │   ├── lcd.h       # LCD 头文件
│       │   └── lcdfont.h   # 字体数据
│       ├── RMT_RX/
│       │   ├── rmt_nec_rx.c    # NEC 协议解码驱动
│       │   └── rmt_nec_rx.h    # RMT 头文件
│       ├── SPI/
│       │   ├── spi.c       # SPI 总线驱动
│       │   └── spi.h       # SPI 头文件
│       ├── MyIIC/
│       │   ├── myiic.c     # I2C 主机驱动
│       │   └── myiic.h     # I2C 头文件
│       ├── XL9555/
│       │   ├── xl9555.c    # IO 扩展芯片驱动
│       │   └── xl9555.h    # XL9555 头文件
│       └── LED/
│           ├── led.c       # LED 驱动
│           └── led.h       # LED 头文件
├── CMakeLists.txt
├── sdkconfig
└── partitions-16MiB.csv
```

## 按键映射 (NEC 协议)

| 按键   | 命令码  | 显示       |
|--------|---------|------------|
| POWER  | 0xBA    | POWER      |
| UP     | 0xB9    | UP         |
| DOWN   | 0xEA    | DOWN       |
| LEFT   | 0xB8    | Veaba      |
| RIGHT  | 0xBC    | FORWARD    |
| BACK   | 0xBB    | BACK       |
| PLAY   | 0xBF    | PLAY/PAUSE |
| VOL+   | 0xF6    | VOL+       |
| VOL-   | 0xF8    | VOL-       |
| 0-9    | 按键值  | 数字显示   |
| DELETE | 0xB5    | DELETE     |

## 初始化顺序

1. LED 初始化
2. I2C0 初始化 (MyIIC)
3. SPI2 初始化
4. XL9555 初始化
5. LCD 初始化
6. RMT NEC 红外接收初始化

## 构建与烧录

```bash
idf.py build
idf.py flash monitor
```