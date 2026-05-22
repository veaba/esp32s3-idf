# 04_uart

> 宇宙为你闪烁

## 项目概述

**驱动方式**: UART 通信 + 轮询

**核心功能**:
- 初始化 UART0 (115200 波特率)
- 接收PC发送的数据并回显
- 定期打印提示信息
- 周期性切换 LED 状态

## 硬件连接

| 信号 | GPIO | 说明 |
|------|------|------|
| LED | GPIO1 | 连接到 LED (低电平点亮) |
| TX | GPIO43 | UART 发送 (连接 PC RX) |
| RX | GPIO44 | UART 接收 (连接 PC TX) |

## 软件架构

```
app_main()
  ├── led_init()                // 初始化 LED GPIO1
  ├── uart_init(115200)         // 初始化 UART0
  └── while(1)
        ├── 检查 UART 缓冲区是否有数据
        │     └── 有数据 → 回显收到的数据
        └── 无数据
              ├── 每 5000 次循环打印分隔线
              ├── 每 200 次循环打印提示语
              └── 每 20 次循环切换 LED
```

## 关键代码

### main.c

```c
void app_main(void)
{
    uint8_t len = 0;
    uint16_t times = 0;
    unsigned char data[RX_BUF_SIZE] = {0};

    led_init();
    uart_init(115200);

    while (1)
    {
        uart_get_buffered_data_len(UART_UX, (size_t *)&len);

        if (len > 0)
        {
            memset(data, 0, RX_BUF_SIZE);
            printf("\n esp32s3 >: ↓ \n ");
            uart_read_bytes(UART_UX, data, len, 100);
            uart_write_bytes(UART_UX, (const char *)data,
                           strlen((const char *)data));
        }
        else
        {
            times++;
            if (times % 5000 == 0)
                printf("\n ------esp32s3 examples------ ");
            if (times % 200 == 0)
                printf("\n 领导！你发下话好吗？求你了");
            if (times % 20 == 0)
                LED_TOGGLE();

            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}
```

### uart.c

```c
void uart_init(uint32_t baud_rate)
{
    uart_config_t uart_config = {0};

    uart_config.baud_rate = baud_rate;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.rx_flow_ctrl_thresh = 122;
    uart_config.source_clk = UART_SCLK_APB;

    uart_param_config(UART_UX, &uart_config);
    uart_set_pin(UART_UX, UART_TX_GPIO_PIN, UART_RX_GPIO_PIN,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_UX, RX_BUF_SIZE * 2, RX_BUF_SIZE * 2,
                       20, NULL, 0);
}
```

### uart.h

```c
#define UART_UX UART_NUM_0
#define UART_TX_GPIO_PIN GPIO_NUM_43
#define UART_RX_GPIO_PIN GPIO_NUM_44
#define RX_BUF_SIZE 1024

void uart_init(uint32_t baud_rate);
```

## API 说明

| 函数 | 说明 |
|------|------|
| `led_init()` | 初始化 LED GPIO (GPIO1) |
| `uart_init(baud_rate)` | 初始化 UART0 |
| `uart_get_buffered_data_len()` | 获取接收缓冲区数据长度 |
| `uart_read_bytes()` | 读取数据 |
| `uart_write_bytes()` | 发送数据 |
| `LED_TOGGLE()` | 切换 LED 状态 |

## UART 配置参数

| 参数 | 值 | 说明 |
|------|-----|------|
| 波特率 | 115200 | 串口通信速率 |
| 数据位 | 8 | 8 位数据 |
| 校验位 | None | 无校验 |
| 停止位 | 1 | 1 位停止位 |
| 流控制 | None | 无硬件流控 |
| 缓冲区 | 2048 bytes | TX/RX 各 2KB |

## 交互说明

1. **连接设备**: 通过 USB 连接 ESP32S3 开发板
2. **打开串口**: 使用串口工具 (如 minicom, putty, Arduino Serial Monitor)
3. **波特率**: 115200
4. **发送数据**: PC 发送任意字符，ESP32 会回显
5. **接收提示**: 无数据时，每隔约 2 秒显示 "------esp32s3 examples------"
6. **LED 闪烁**: 无数据时，LED 每 200ms 切换一次

## 性能特点

| 指标 | 值 |
|------|-----|
| 波特率 | 115200 bps |
| CPU 占用 | 持续轮询 (高) |
| 功耗 | 较高 |
| LED 闪烁周期 | ~200ms |

## 优缺点

**优点**:
- 实现了完整的串口收发功能
- 代码结构清晰，功能分区明确
- 串口参数可灵活配置

**缺点**:
- 使用轮询方式检查串口数据，CPU 占用高
- 缓冲区设置较大，占用内存
- 定时打印消息增加了 CPU 负载

## 优化方向

1. **使用中断驱动 UART**: 替代轮询，减少 CPU 占用
2. **使用 FreeRTOS 队列**: 解耦收发逻辑
3. **减少定时打印**: 降低串口输出频率
4. **使用事件组**: 协调 UART 事件和 LED 任务

## 原理图

> file:///E:/%E7%A1%AC%E4%BB%B6%E7%B1%BB%E8%BD%AF%E4%BB%B6/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91DNESP32S3%E5%BC%80%E5%8F%91%E6%9D%BF/%E3%80%90%E6%AD%A3%E7%82%B9%E5%8E%9F%E5%AD%90%E3%80%91DNESP32S3%E5%BC%80%E5%8F%91%E6%9D%BF%E8%B5%84%E6%96%99%EF%BC%88A%E7%9B%98%EF%BC%89/3%EF%BC%8C%E5%8E%9F%E7%90%86%E5%9B%BE/ATK_DNESP32S3_V1.2.pdf

## 编译烧录

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```