# ESP32-S3 EspTouch v2 配网协议 — C 端实现详析

> 本文档为 `19_wifi_auto_esptouch` 项目的 SmartConfig/EspTouch v2 配网逻辑的完整技术参考。
>
> **当前配置**：`SC_TYPE_ESPTOUCH_V2` — EspTouch v2 协议（支持 Reserved Data + 可选 AES-128 加密）
>
> EspTouchForHarmony APP 侧应严格参照本文档中描述的协议流程、数据结构、事件时序和交互细节进行对接实现。

---

## 1 协议概述

### 1.1 EspTouch 是什么

EspTouch 是 Espressif 提供的 **SmartConfig** WiFi 配网方案。手机 APP 将 WiFi 的 SSID、密码等信息编码到 802.11 广播/组播帧的**包长度**中，ESP32 设备在混杂模式下抓取空中无线帧，解析出 SSID 和密码后连接路由器。

### 1.2 EspTouch 版本对照

| 版本 | 协议常量 | 说明 |
|------|---------|------|
| EspTouch v1 | `SC_TYPE_ESPTOUCH` (0) | 原始协议，只传递 SSID + 密码 |
| AirKiss | `SC_TYPE_AIRKISS` (1) | 微信配网协议，只传递 SSID + 密码 |
| EspTouch + AirKiss | `SC_TYPE_ESPTOUCH_AIRKISS` (2) | 同时监听两种协议 |
| **EspTouch v2** | `SC_TYPE_ESPTOUCH_V2` (3) | **v2 协议，在 v1 基础上增加 reserved data (自定义数据)** |

### 1.3 EspTouch v2 与 v1 的关键区别

| 特性 | v1 | v2 |
|------|----|----|
| 传递 SSID | ✅ | ✅ |
| 传递 Password | ✅ | ✅ |
| 传递 BSSID | ✅ | ✅ |
| **Reserved Data (自定义数据)** | ❌ | ✅ 最多 64 字节 |
| **AES-128 加密** | ❌ | ✅ 可选，密钥固定 16 字节 |
| ACK 机制 | ✅ | ✅ |

> **APP 对接要点**：EspTouch v2 在 v1 基础上新增了 `reserved data` 字段和可选的 AES-128 加密能力，这是 APP 侧需要重点实现的差异点。

---

## 2 C 端整体架构

### 2.1 文件结构

```
19_wifi_auto_esptouch/
├── main/
│   └── main.c                    # 程序入口：初始化各外设 → 调用 wifi_smart_init()
├── components/BSP/
│   └── MYWIFI/
│       ├── my_wifi.h             # WiFi 配网 API 声明、宏定义、默认凭据
│       └── my_wifi.c             # WiFi 配网核心逻辑（事件处理、smartconfig_task、连接流程）
├── sdkconfig                     # ESP-IDF 编译配置
├── CMakeLists.txt                # 顶层 CMake 配置
└── partitions-16MiB.csv          # 16MiB 分区表
```

### 2.2 关键头文件依赖

C 端使用了以下 ESP-IDF 核心 API：

| 头文件 | 提供的 API |
|--------|-----------|
| `esp_smartconfig.h` | `esp_smartconfig_set_type()`, `esp_smartconfig_start()`, `esp_smartconfig_stop()`, `esp_smartconfig_get_rvd_data()`, `smartconfig_start_config_t`, `smartconfig_event_got_ssid_pswd_t` |
| `esp_wifi.h` | `esp_wifi_init()`, `esp_wifi_set_mode()`, `esp_wifi_start()`, `esp_wifi_connect()`, `esp_wifi_disconnect()`, `esp_wifi_set_config()`, `wifi_init_config_t` |
| `esp_event.h` | `esp_event_loop_create_default()`, `esp_event_handler_register()`, 事件基 `WIFI_EVENT` / `IP_EVENT` / `SC_EVENT` |
| `esp_netif.h` | `esp_netif_init()`, `esp_netif_create_default_wifi_sta()` |
| `nvs_flash.h` | `nvs_flash_init()`, `nvs_flash_erase()` |
| `freertos/FreeRTOS.h` | `EventGroupHandle_t`, `xEventGroupCreate()`, `xEventGroupSetBits()`, `xEventGroupWaitBits()`, `xEventGroupClearBits()` |

### 2.3 宏定义与默认配置

来自 `my_wifi.h`：

```c
#define DEFAULT_SSID      "iPhone17"          // 默认 SSID（仅备用）
#define DEFAULT_PASSWORD  "123456789"         // 默认密码（仅备用）
#define CONNECTED_BIT     BIT0                // EventGroup 位0：WiFi 已连接并获取 IP
#define ESPTOUCH_DONE_BIT BIT1               // EventGroup 位1：SmartConfig 流程完成
```

### 2.4 BSP 组件依赖

```
CMakeLists.txt (components/BSP/) 中 REQUIRES:
  - driver        # GPIO、SPI 等外设驱动
  - esp_lcd       # LCD 显示驱动
  - esp_wifi      # WiFi + SmartConfig 功能
```

编译选项：`-ffast-math -O3 -Wno-error=format=-Wno-format`

---

## 3 完整启动时序（从上电到配网成功）

### 3.1 时序图

```
app_main()
  │
  ├─ flash_init()                     // ① NVS Flash 初始化（WiFi 凭据存储必需）
  ├─ led_init()                       // ② LED 初始化
  ├─ my_spi_init()                    // ③ SPI 总线初始化（LCD 用）
  ├─ myiic_init()                     // ④ I2C 总线初始化（XL9555 IO 扩展用）
  ├─ spilcd_init()                    // ⑥ SPI LCD 初始化
  │
  ├─ LCD 显示 "ESP32-S3" + "WiFi Smart Config Network"
  │
  └─ wifi_smart_init()                // ⑦ WiFi 配网主入口
       │
       ├─ xEventGroupCreate()         // 创建事件组
       ├─ esp_netif_init()            // 网卡初始化
       ├─ esp_event_loop_create_default() // 事件循环
       ├─ esp_netif_create_default_wifi_sta() // 创建 STA 网卡
       ├─ esp_wifi_init(&cfg)         // WiFi 驱动默认配置初始化
       │
       ├─ 注册 3 组事件处理器:
       │   ├─ WIFI_EVENT  → wifi_event_handler
       │   ├─ IP_EVENT    → wifi_event_handler
       │   └─ SC_EVENT    → wifi_event_handler
       │
       ├─ esp_wifi_set_mode(WIFI_MODE_STA)   // 设置 STA 模式
       ├─ esp_wifi_start()                     // 启动 WiFi
       │
       │  ┌──────────────────────────────────────┐
       │  │ 系统自动触发 WIFI_EVENT_STA_START     │
       │  │ → wifi_event_handler                  │
       │  │ → xTaskCreate(smartconfig_task, ...)  │
       │  └──────────────────────────────────────┘
       │
       └─ xEventGroupWaitBits(CONNECTED_BIT | ESPTOUCH_DONE_BIT)
            │
            ├─ CONNECTED_BIT 置位 → "Connected to ap"
            └─ ESPTOUCH_DONE_BIT → 连接失败
```

### 3.2 smartconfig_task 详细流程

```
smartconfig_task()
  │
  ├─ esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_V2)  // 设置 EspTouch v2 配网协议
  │
  ├─ SMARTCONFIG_START_CONFIG_DEFAULT()              // 默认配网参数
  │    → .enable_log = false
  │    → .esp_touch_v2_enable_crypt = false           // v2 加密：未启用（可按需开启）
  │    → .esp_touch_v2_key = NULL                     // 加密密钥：未设置
  │
  ├─ esp_smartconfig_start(&cfg)                     // 启动 SmartConfig
  │
  └─ while(1) 循环等待:
       xEventGroupWaitBits(CONNECTED_BIT | ESPTOUCH_DONE_BIT)
         │
         ├─ CONNECTED_BIT    → "WiFi connected to AP"
         └─ ESPTOUCH_DONE_BIT → esp_smartconfig_stop() + vTaskDelete(NULL)

⚠️ 重要：协议类型必须与 APP 端一致
   - C 端: SC_TYPE_ESPTOUCH_V2 → APP 必须使用 EspTouch v2 协议发送
   - 如 APP 使用 EspTouch v1 发送，配网将失败
   - 如需 AES 加密，需同时设置 cfg.esp_touch_v2_enable_crypt = true 和 cfg.esp_touch_v2_key
```

---

## 4 事件处理完整流程 — wifi_event_handler

这是 C 端核心，APP 侧需理解每个事件的触发时机和数据结构。

### 4.1 事件分发总览

```
wifi_event_handler(arg, event_base, event_id, event_data)
  │
  ├─ event_base == WIFI_EVENT
  │   ├─ WIFI_EVENT_STA_START       → 创建 smartconfig_task 任务
  │   ├─ WIFI_EVENT_STA_DISCONNECTED → 重连 + 清除 CONNECTED_BIT
  │   └─ WIFI_EVENT_STA_CONNECTED   → LCD 显示连接成功
  │
  ├─ event_base == IP_EVENT
  │   └─ IP_EVENT_STA_GOT_IP        → 获取到IP → 置位 CONNECTED_BIT
  │
  └─ event_base == SC_EVENT
      ├─ SC_EVENT_SCAN_DONE         → 扫描完成，等待配网
      ├─ SC_EVENT_GOT_SSID_PSWD    → 收到 SSID+密码（核心事件）
      └─ SC_EVENT_SEND_ACK_DONE     → ACK 发送完成 → 置位 ESPTOUCH_DONE_BIT
```

### 4.2 SC_EVENT_GOT_SSID_PSWD — 核心配网数据接收

这是 APP 与 ESP32 交互的关键事件。当 ESP32 在混杂模式下成功解码出手机发来的配网数据时触发。

#### 4.2.1 事件数据结构

```c
typedef struct {
    uint8_t ssid[32];           // SSID，null-terminated 字符串
    uint8_t password[64];       // 密码，null-terminated 字符串
    bool    bssid_set;          // 是否指定了目标 AP 的 BSSID
    uint8_t bssid[6];           // 目标 AP 的 MAC 地址
    smartconfig_type_t type;    // 配网协议类型
    uint8_t token;              // 手机端的 token，用于 ACK 回传
    uint8_t cellphone_ip[4];    // 手机的 IP 地址（4 字节，网络字节序）
} smartconfig_event_got_ssid_pswd_t;
```

**APP 对接要点**：

- `ssid[32]`：最大 32 字节，实际是 null-terminated 字符串
- `password[64]`：最大 64 字节，WPA2 密码最长 63 字符 + '\0'
- `bssid_set`：当手机连接的路由器有多个 BSSID 时，APP 应设置此字段为 `true` 并填充 `bssid`，确保设备连到正确 AP
- `cellphone_ip[4]`：这是手机的局域网 IP 地址，ACK 回传时使用
- `token`：手机 APP 生成的标识，ESP32 回传 ACK 时携带此 token

#### 4.2.2 C 端处理逻辑

```c
case SC_EVENT_GOT_SSID_PSWD: {
    smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
    wifi_config_t wifi_config;
    uint8_t ssid[33] = {0};       // 32 + 1 for null terminator
    uint8_t password[65] = {0};   // 64 + 1 for null terminator
    uint8_t rvd_data[65] = {0};   // reserved data buffer

    bzero(&wifi_config, sizeof(wifi_config_t));
    memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
    memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
    wifi_config.sta.bssid_set = evt->bssid_set;

    if (wifi_config.sta.bssid_set == true) {
        memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
    }

    memcpy(ssid, evt->ssid, sizeof(evt->ssid));
    memcpy(password, evt->password, sizeof(evt->password));
    ESP_LOGI(TAG, "SSID:%s", ssid);
    ESP_LOGI(TAG, "PASSWORD:%s", password);

// EspTouch v2: 读取 reserved data (自定义数据)
        // 当前协议类型为 SC_TYPE_ESPTOUCH_V2，此条件必定为真
        if (evt->type == SC_TYPE_ESPTOUCH_V2) {
        ESP_ERROR_CHECK(esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)));
        // rvd_data 中包含 APP 端传入的自定义数据（最多 64 字节）
        for (int i = 0; i < 32; i++) {
            printf("%02x", rvd_data[i]);
        }
        printf("\n");
    }

    // 断开 → 重写配置 → 重连
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    esp_wifi_connect();
    break;
}
```

### 4.3 EspTouch v2 Reserved Data 详解

#### 4.3.1 API 说明

```c
esp_err_t esp_smartconfig_get_rvd_data(uint8_t *rvd_data, uint8_t len);
```

- **`rvd_data`**：输出缓冲区，接收 APP 传入的自定义数据
- **`len`**：缓冲区长度，最大 64 字节
- **返回值**：`ESP_OK` 成功，其他失败
- **必须在 `SC_EVENT_GOT_SSID_PSWD` 事件中使用**，在其他上下文中调用无效
- **仅在 `evt->type == SC_TYPE_ESPTOUCH_V2` 时才有数据**

> ⚠️ **重要**：SSID/密码数据在 `SC_EVENT_GOT_SSID_PSWD` 事件中获取，**不是** `SC_EVENT_FOUND_CHANNEL`。
> `SC_EVENT_FOUND_CHANNEL` 仅表示找到了信道，其 `event_data` 为空，访问会导致空指针崩溃。

#### 4.3.2 Reserved Data 的用途（APP 侧可传递的数据）

| 用途 | 说明 | 长度 |
|------|------|------|
| 设备绑定信息 | APP 用户 ID、设备归属 | 变长 |
| 自定义协议数据 | 如 MQTT broker 地址、端口 | 变长 |
| 加密密钥/令牌 | 后续通信用的密钥材料 | 固定长度 |
| 设备初始配置 | 设备工作模式、参数等 | 变长 |

> **APP 对接要点**：Reserved Data 最大 64 字节。APP 端通过 EspTouch v2 协议编码时传入，设备端通过 `esp_smartconfig_get_rvd_data()` 解码获取。

---

## 5 EspTouch v2 加密机制

### 5.1 配置结构体

```c
typedef struct {
    bool enable_log;                 // 是否启用 SmartConfig 日志
    bool esp_touch_v2_enable_crypt;  // 是否启用 EspTouch v2 AES-128 加密
    char *esp_touch_v2_key;          // AES-128 密钥，必须恰好 16 字节
} smartconfig_start_config_t;
```

### 5.2 默认配置宏

```c
#define SMARTCONFIG_START_CONFIG_DEFAULT() { \
    .enable_log = false,                       \
    .esp_touch_v2_enable_crypt = false,        \
    .esp_touch_v2_key = NULL                   \
}
```

### 5.3 加密流程

```
┌──────────────┐                              ┌──────────────┐
│   手机 APP    │                              │    ESP32     │
└──────┬───────┘                              └──────┬───────┘
       │                                            │
       │  1. AES-128 加密 SSID + 密码 + RVD Data   │
       │     使用共享密钥 (16 字节)                  │
       │                                            │
       │  2. 编码加密数据到广播包长度                   │
       │                                            │
       │  ────── EspTouch v2 编码帧 ──────►         │
       │                                            │  3. 混杂模式接收
       │                                            │  4. 从包长度解码数据
       │                                            │  5. AES-128 解密
       │                                            │     使用同一密钥
       │                                            │  6. 得到 SSID + 密码 + RVD Data
       │                                            │
       │  ◄──── ACK (含 token + cellphone_ip) ──   │
       │                                            │
```

### 5.4 当前项目的加密配置

**当前项目未启用加密，使用 EspTouch v2 明文模式**：

```c
esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_V2);                    // v2 协议
smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT(); // 默认参数
// cfg.esp_touch_v2_enable_crypt = false  (默认值，未启用加密)
// cfg.esp_touch_v2_key = NULL            (默认值)
```

当前配置含义：

- 协议类型：EspTouch v2（支持 Reserved Data）
- 加密：**未启用**（APP 发送明文配网数据即可）
- Reserved Data：**可用**（APP 可传最多 64 字节自定义数据）

> **APP 对接要点**：当前 C 端使用 EspTouch v2 明文模式。APP 发送配网数据时无需加密，但必须使用 v2 协议编码格式。如 C 端改为启用加密（`esp_touch_v2_enable_crypt = true`），APP 必须使用**完全相同的 16 字节 AES-128 密钥**加密。

### 5.5 启用加密的代码示例

```c
// 启用 EspTouch v2 + AES 加密
esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_V2);

smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
cfg.esp_touch_v2_enable_crypt = true;
cfg.esp_touch_v2_key = "1234567890abcdef";  // 必须 16 字节

ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));
```

---

## 6 状态机与事件位

### 6.1 FreeRTOS EventGroup 两位定义

| 事件位 | 值 | 含义 | 设置时机 | 清除时机 |
|--------|---|------|---------|---------|
| `CONNECTED_BIT` | `BIT0` (0x01) | WiFi 已连接并获取 IP | `IP_EVENT_STA_GOT_IP` | `WIFI_EVENT_STA_DISCONNECTED` |
| `ESPTOUCH_DONE_BIT` | `BIT1` (0x02) | SmartConfig 流程结束 | `SC_EVENT_SEND_ACK_DONE` | 不会清除 |

### 6.2 完整状态流转

```
                    ┌────────────┐
                    │   app_main  │
                    └─────┬──────┘
                          │ flash_init, 外设初始化
                          ▼
                    ┌────────────┐
                    │wifi_smart  │
                    │  _init()   │
                    └─────┬──────┘
                          │ 初始化 netif, event loop, WiFi STA
                          │ 注册事件处理器
                          │ esp_wifi_start()
                          ▼
              ┌───────────────────────┐
              │  WIFI_EVENT_STA_START │
              └───────────┬──────────┘
                          │ xTaskCreate(smartconfig_task)
                          ▼
┌───────────────────────┐
               │   smartconfig_task    │◄─────────────────────────────┐
               └───────────┬──────────┘                               │
                           │ esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_V2)│
                           │ esp_smartconfig_start()                  │
                          ▼                                         │
              ┌───────────────────────┐                             │
              │  SC_EVENT_SCAN_DONE   │ ← ESP32扫描AP完成            │
              └───────────┬──────────┘                             │
                          ▼                                         │
              ┌───────────────────────┐                             │
              │SC_EVENT_GOT_SSID_PSWD│ ← 收到APP配网数据            │
              └───────────┬──────────┘                             │
                          │ 获取 SSID, 密码, [RVD Data]             │
                          │ esp_wifi_disconnect()                   │
                          │ esp_wifi_set_config()                   │
                          │ esp_wifi_connect()                      │
                          ▼                                         │
         ┌──────────────────────────────┐                          │
         │                              │                          │
    ┌────▼─────┐              ┌─────────▼─────────┐               │
    │ 连接成功  │              │  连接失败/断开      │               │
    └────┬─────┘              └─────────┬─────────┘               │
         │                              │                         │
         ▼                              │ esp_wifi_connect()     │
  ┌────────────────┐                    │ (自动重连)               │
  │IP_EVENT_STA_   │◄───────────────────┘                         │
  │  GOT_IP        │                                              │
  └───────┬────────┘                                              │
          │ CONNECTED_BIT = BIT0                                    │
          ▼                                                         │
  ┌────────────────────┐                                           │
  │SC_EVENT_SEND_ACK_  │                                           │
  │     DONE           │                                           │
  └────────┬───────────┘                                           │
           │ ESPTOUCH_DONE_BIT = BIT1                                │
           ▼                                                         │
  ┌────────────────────┐                                            │
  │ smartconfig_stop() │                                            │
  │ vTaskDelete(NULL)  │────────────────────────────────────────────┘
  └────────────────────┘  (任务自删除)
```

---

## 7 当前协议配置与注意事项

### 7.1 当前配置

```c
esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_V2);   // ✅ 使用 EspTouch v2 协议
```

事件处理中同步检查 v2 类型以读取 Reserved Data：

```c
if (evt->type == SC_TYPE_ESPTOUCH_V2) {
    ESP_ERROR_CHECK(esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)));
    // ... 处理 Reserved Data
}
```

协议类型与事件处理**已匹配**，`esp_smartconfig_get_rvd_data()` 可正确执行。

### 7.2 协议类型与 APP 端的对应关系

| C 端 `esp_smartconfig_set_type()` | APP 必须使用的协议 | Reserved Data | AES 加密 | 配网结果 |
|------|------|------|------|------|
| `SC_TYPE_ESPTOUCH` | EspTouch v1 | ❌ | ❌ | 类型不匹配，勿使用 |
| `SC_TYPE_AIRKISS` | AirKiss | ❌ | ❌ | 类型不匹配，勿使用 |
| `SC_TYPE_ESPTOUCH_AIRKISS` | EspTouch v1 或 AirKiss | ❌ | ❌ | 类型不匹配，勿使用 |
| **`SC_TYPE_ESPTOUCH_V2`** ← 当前 | **EspTouch v2** | ✅ | ✅ 可选 | ✅ 必须使用此组合 |

> **APP 对接要点**：C 端当前固定使用 `SC_TYPE_ESPTOUCH_V2`，APP 必须也使用 EspTouch v2 协议发送配网数据。如果 APP 使用 v1 协议，ESP32 将无法解码，配网失败。

### 7.3 启用 AES-128 加密（可选）

当前项目未启用加密（`esp_touch_v2_enable_crypt = false`）。如需启用：

```c
esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_V2);     // 必须 v2 才能加密

smartconfig_start_config_t cfg = {
    .enable_log = false,
    .esp_touch_v2_enable_crypt = true,               // 启用 AES-128 加密
    .esp_touch_v2_key = "1234567890abcdef",           // 16 字节密钥
};
ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));
```

启用加密后，APP 也必须使用**完全相同的 16 字节密钥**加密配网数据。

---

## 8 APP 侧对接要点汇总

### 8.1 EspTouch v2 协议数据包结构

**当前 C 端使用 `SC_TYPE_ESPTOUCH_V2`**，APP 端必须使用 v2 协议编码。

APP 端通过 EspTouch v2 协议发送的数据包含以下字段：

| 字段 | 长度 | 说明 |
|------|------|------|
| SSID | 1~32 字节 | 目标 WiFi 名称，null-terminated |
| Password | 1~64 字节 | WiFi 密码，null-terminated |
| BSSID | 6 字节 | 目标 AP 的 MAC 地址（可选） |
| BSSID_Set | 1 字节 (bool) | 是否指定 BSSID |
| Token | 1 字节 | 手机端标识，ACK 回传用 |
| Cellphone IP | 4 字节 | 手机 IP 地址 |
| **Reserved Data** | 0~64 字节 | **v2 独有**：自定义数据 |

### 8.2 ACK 回传机制

1. ESP32 解码出 SSID + 密码后，通过 UDP 回传 ACK 给手机
2. ACK 数据包含：`token`（来自 APP）+ `cellphone_ip`（来自 APP）
3. ACK 目标地址：`cellphone_ip` 指定的手机 IP
4. APP 收到 ACK 后确认配网成功
5. `SC_EVENT_SEND_ACK_DONE` 事件触发后，ESP32 置位 `ESPTOUCH_DONE_BIT`

### 8.3 加密对接

| 场景 | C 端配置 | APP 端要求 |
|------|---------|-----------|
| **不加密（当前配置）** | `esp_touch_v2_enable_crypt = false` | 直接明文传输，使用 v2 编码 |
| AES-128 加密 | `esp_touch_v2_enable_crypt = true`, `esp_touch_v2_key = "1234567890abcdef"` | 必须使用**完全相同**的 16 字节密钥加密 |

> 两种场景都要求 APP 使用 EspTouch **v2** 编码格式（因为 C 端固定为 `SC_TYPE_ESPTOUCH_V2`）。

### 8.4 协议类型对接矩阵

| C 端 `esp_smartconfig_set_type()` | APP 应使用的协议 | Reserved Data | 加密支持 | 当前是否使用 |
|------|------|------|------|------|
| `SC_TYPE_ESPTOUCH` | EspTouch v1 | ❌ | ❌ | ❌ |
| `SC_TYPE_AIRKISS` | AirKiss | ❌ | ❌ | ❌ |
| `SC_TYPE_ESPTOUCH_AIRKISS` | EspTouch v1 或 AirKiss | ❌ | ❌ | ❌ |
| **`SC_TYPE_ESPTOUCH_V2`** | **EspTouch v2** | ✅ | ✅ 可选 | ✅ **当前配置** |

> **APP 必须使用 EspTouch v2 协议**，其他协议类型将导致配网失败。

### 8.5 APP 侧实现流程（EspTouch v2）

> **C 端当前配置**：`SC_TYPE_ESPTOUCH_V2` + 无加密 + Reserved Data 可用

```
1. 用户在 APP 中输入 WiFi SSID 和密码
2. 可选：输入 Reserved Data（自定义数据，最大 64 字节）
   → C 端通过 esp_smartconfig_get_rvd_data() 获取
3. 可选：输入 AES-128 加密密钥（16 字节）
   → 当前 C 端未启用加密，APP 发送明文即可
   → 如 C 端启用加密，APP 必须使用相同 16 字节密钥
4. APP 使用 EspTouch v2 编码方式发送广播帧
   - 编码方式：将数据映射到 802.11 帧的包长度中
   - 加密（如启用）：先 AES-128 加密，再编码
5. 等待 ESP32 的 ACK 回传（UDP 单播到手机 IP）
6. 收到 ACK 后，APP 可以关闭配网界面
7. ESP32 此时已获得 WiFi 凭据并开始连接路由器
```

### 8.6 Reserved Data 的 APP 端编码规则

**C 端已使用 `SC_TYPE_ESPTOUCH_V2`，Reserved Data 功能已激活。**

APP 通过 EspTouch v2 发送 Reserved Data 时：

- 数据在 SSID + Password 之后追加
- 总长度不超过编码协议的最大帧长
- 数据在传输时可被 AES-128 加密（与 SSID/Password 使用同一密钥）
- ESP32 端整体解密后，通过 `esp_smartconfig_get_rvd_data()` 单独提取
- 当前 C 端未启用加密，APP 直接明文编码 Reserved Data 即可

Reserved Data 典型用途：

| 用途 | 说明 | 字节数建议 |
|------|------|-----------|
| 设备绑定 Token | APP 用户标识，设备绑定到特定用户 | 8~32 字节 |
| MQTT/服务器信息 | broker 地址 + 端口号 | 变长 |
| 自定义配置 | 设备工作模式、参数 | 变长 |

C 端读取示例（当前代码仅 hex dump 前 32 字节）：

```c
uint8_t rvd_data[65] = {0};
ESP_ERROR_CHECK(esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)));
// rvd_data[0..63]: APP 端传入的自定义数据（实际有效长度取决于 APP）
// rvd_data[64]: 不会超过 64 字节
```

---

## 9 WiFi 事件详细参考

### 9.1 WIFI_EVENT 事件

| event_id | 名称 | 触发条件 | C 端处理 |
|----------|------|---------|---------|
| `WIFI_EVENT_STA_START` | STA 模式启动 | `esp_wifi_start()` 后 | 创建 `smartconfig_task` |
| `WIFI_EVENT_STA_DISCONNECTED` | STA 断开 | 连接丢失 | 调用 `esp_wifi_connect()` 重连 |
| `WIFI_EVENT_STA_CONNECTED` | STA 连接成功 | 关联成功 | LCD 显示"连接成功" |

### 9.2 IP_EVENT 事件

| event_id | 名称 | 触发条件 | C 端处理 |
|----------|------|---------|---------|
| `IP_EVENT_STA_GOT_IP` | 获取 IP | DHCP 分配 IP 成功 | 置位 `CONNECTED_BIT` |

### 9.3 SC_EVENT 事件

| event_id | 名称 | 触发条件 | C 端处理 |
|----------|------|---------|---------|
| `SC_EVENT_SCAN_DONE` | 扫描完成 | ESP32 完成信道扫描 | LCD 显示"配网中..." |
| `SC_EVENT_FOUND_CHANNEL` | 找到信道 | 锁定AP所在信道 | 仅通知，event_data 为空 |
| `SC_EVENT_GOT_SSID_PSWD` | 获取SSID密码 | 解码出SSID+密码 | **核心处理**（见 4.2 节） |
| `SC_EVENT_SEND_ACK_DONE` | ACK 发送完成 | ACK 已发送到手机 | 置位 `ESPTOUCH_DONE_BIT` |

---

## 10 完整代码清单

### 10.1 my_wifi.h

```c
#ifndef __MY_WIFI_H
#define __MY_WIFI_H

#include <stdint.h>

#define CONNECTED_BIT     BIT0                // EventGroup 位0：WiFi 已连接并获取 IP
#define ESPTOUCH_DONE_BIT BIT1                // EventGroup 位1：SmartConfig 流程结束

#define WIFI_NOTIFY_PORT    7001    // UDP 反馈端口
#define WIFI_NOTIFY_COUNT   3       // UDP 广播次数
#define WIFI_NOTIFY_INTERVAL_MS 500 // UDP 广播间隔(ms)
#define WIFI_RETRY_MAX      5       // WiFi 断线最大重试次数

#define WIFI_NOTIFY_STATUS_SUCCESS    0   // WiFi 连接成功
#define WIFI_NOTIFY_STATUS_FAIL       1   // WiFi 连接失败
#define WIFI_NOTIFY_STATUS_DISCONNECT 2   // 用户主动断开

void wifi_smart_init(void);
void wifi_forget_and_reconfig(void);
void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void draw_ascii_char(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg_color);
#endif
```

### 10.2 my_wifi.c（核心逻辑）

```c
#include "my_wifi.h"
#include "chinese16_3k.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif_types.h"
#include "esp_smartconfig.h"
#include "esp_wifi.h"
#include "esp_wifi_types_generic.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/task.h"
#include "spilcd.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static const char *TAG = "【smartconfig_task】";
static EventGroupHandle_t s_wifi_event_group;

// 事件位定义
// CONNECTED_BIT     = BIT0 = 0x01  WiFi已连接并获取IP
// ESPTOUCH_DONE_BIT = BIT1 = 0x02  SmartConfig流程结束

static void smartconfig_task(void *parm);
static char lcd_buff[100] = {0};

void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color) { spilcd_draw_point(x, y, color); }

void draw_ascii_char(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg_color) {
  spilcd_show_char(x, y, (uint8_t)c, 16, 0, color);
}

void connect_display(uint8_t flag) {
  if (flag == 2) {
    spilcd_fill(0, 110, 320, 240, WHITE);
    sprintf(lcd_buff, "【ssid】:%s", DEFAULT_SSID);
    spilcd_show_string(0, 150, 240, 16, 16, lcd_buff, WHITE);
    sprintf(lcd_buff, "【psw】:%s", DEFAULT_PASSWORD);
    spilcd_show_string(0, 150, 240, 16, 16, lcd_buff, WHITE);
  } else if (flag == 1) {
    spilcd_show_string(0, 150, 240, 16, 16, "wifi connect fail", RED);
  } else {
    spilcd_show_string(0, 150, 240, 16, 16, "wifi connecting...", BLUE);
  }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT) {
    switch (event_id) {
    case WIFI_EVENT_STA_START:
      connect_display(0);
      xTaskCreate(smartconfig_task, "smartconfig_task", 4096, NULL, 3, NULL);
      break;
    case WIFI_EVENT_STA_DISCONNECTED:
      esp_wifi_connect();
      xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
      ESP_LOGI(TAG, "connect to the AP fail");
      break;
    case WIFI_EVENT_STA_CONNECTED:
      connect_display(2);
      break;
    default:
      break;
    }
  } else if (event_base == IP_EVENT) {
    if (event_id == IP_EVENT_STA_GOT_IP) {
      ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
      ESP_LOGI(TAG, "static ip:" IPSTR, IP2STR(&event->ip_info.ip));
      xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
      show_chinese_string(0, 150, "WIFI 连接成功！", BLUE, WHITE,
                          lcd_draw_pixel, draw_ascii_char);
    }
  } else if (event_base == SC_EVENT) {
    switch (event_id) {
    case SC_EVENT_SCAN_DONE:
      ESP_LOGI(TAG, "scan done.");
      spilcd_show_string(10, 110, 320, 16, 16,
                         "In the distribution network...", BLUE);
      break;

    case SC_EVENT_GOT_SSID_PSWD: {
      ESP_LOGI(TAG, "Got SSID and password");
      smartconfig_event_got_ssid_pswd_t *evt =
          (smartconfig_event_got_ssid_pswd_t *)event_data;
      wifi_config_t wifi_config;
      uint8_t ssid[33] = {0};
      uint8_t password[65] = {0};
      uint8_t rvd_data[65] = {0};

      bzero(&wifi_config, sizeof(wifi_config_t));
      memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
      memcpy(wifi_config.sta.password, evt->password,
             sizeof(wifi_config.sta.password));
      wifi_config.sta.bssid_set = evt->bssid_set;

      if (wifi_config.sta.bssid_set == true) {
        memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
      }

      memcpy(ssid, evt->ssid, sizeof(evt->ssid));
      memcpy(password, evt->password, sizeof(evt->password));
      ESP_LOGI(TAG, "SSID:%s", ssid);
      ESP_LOGI(TAG, "PASSWORD:%s", password);

      spilcd_fill(0, 110, 320, 240, WHITE);
      sprintf(lcd_buff, "%s", ssid);
      spilcd_show_string(10, 110, 320, 16, 16, lcd_buff, BLUE);
      sprintf(lcd_buff, "%s", password);
      spilcd_show_string(10, 110, 320, 16, 16, lcd_buff, BLUE);
      spilcd_show_string(10, 130, 320, 16, 16,
                         "Distribution network is successful.", BLUE);

      if (evt->type == SC_TYPE_ESPTOUCH_V2) {
        ESP_ERROR_CHECK(
            esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)));
        ESP_LOGI(TAG, "RVD_DATA");
        for (int i = 0; i < 32; i++) {
          printf("%02x", rvd_data[i]);
        }
        printf("\n");
      }

      ESP_ERROR_CHECK(esp_wifi_disconnect());
      ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
      esp_wifi_connect();
      break;
    }

    case SC_EVENT_SEND_ACK_DONE:
      xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
      break;
    }
  }
}

static void smartconfig_task(void *parm) {
  parm = parm;
  EventBits_t uxBits;
  ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_V2));      // EspTouch v2 协议
  smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT(); // 默认参数（无加密）
  ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));

  while (1) {
    uxBits = xEventGroupWaitBits(s_wifi_event_group,
                                  CONNECTED_BIT | ESPTOUCH_DONE_BIT,
                                  true, false, portMAX_DELAY);
    if (uxBits & CONNECTED_BIT) {
      ESP_LOGI(TAG, "Wifi connected to AP");
    }
    if (uxBits & ESPTOUCH_DONE_BIT) {
      ESP_LOGI(TAG, "smartconfig over");
      esp_smartconfig_stop();
      vTaskDelete(NULL);
    }
  }
}

void wifi_smart_init(void) {
  show_chinese_string(0, 80, "WIFI 实验", BLUE, WHITE,
                      lcd_draw_pixel, draw_ascii_char);
  s_wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
  assert(sta_netif);

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                              &wifi_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                              &wifi_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID,
                                              &wifi_event_handler, NULL));

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());

  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                          CONNECTED_BIT | ESPTOUCH_DONE_BIT,
                                          pdFALSE, pdFALSE, portMAX_DELAY);
  if (bits & CONNECTED_BIT) {
    ESP_LOGI(TAG, "Connected to ap SSID: %s | password: %s",
            DEFAULT_SSID, DEFAULT_PASSWORD);
  } else if (bits & ESPTOUCH_DONE_BIT) {
    connect_display(1);
    ESP_LOGI(TAG, "Failed to connect to SSID: %s | password: %s",
             DEFAULT_SSID, DEFAULT_PASSWORD);
  } else {
    ESP_LOGI(TAG, "Unexpected event");
  }
}
```

---

## 11 NVS 与 WiFi 凭据存储

### 11.1 NVS 初始化

```c
void flash_init() {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NO_FREE_PAGES) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_ERROR_CHECK(nvs_flash_init());
  }
}
```

NVS (Non-Volatile Storage) 是 ESP32 的键值对持久存储系统。`esp_wifi_set_config()` 会自动将 WiFi 凭据写入 NVS 分区。

### 11.2 WiFi 凭据自动存储

ESP-IDF 的 WiFi 库在 `esp_wifi_set_config()` 时会自动将 SSID 和密码存入 NVS。
下次启动时，如果 NVS 中有已保存的凭据，可以直接连接，无需重新配网。

分区表中 NVS 分区配置（`partitions-16MiB.csv`）：

| 名称 | 类型 | 子类型 | 偏移 | 大小 |
|------|------|--------|------|------|
| nvs | data | nvs | 0x9000 | 0x6000 (24KB) |

---

## 12 超时与重连机制

### 12.1 SmartConfig 超时

```c
esp_err_t esp_esptouch_set_timeout(uint8_t time_s);
```

- 范围：15~255 秒，偏移 45 秒（实际超时 = `time_s + 45`）
- 当前项目**未调用此函数**，使用默认超时
- 超时后 SmartConfig 会自动重启扫描

### 12.2 WiFi 断线重连（含重试上限）

```c
#define WIFI_RETRY_MAX 5  // 最大重试次数

static int s_retry_num = 0;

case WIFI_EVENT_STA_DISCONNECTED: {
    s_retry_num++;
    if (s_retry_num > WIFI_RETRY_MAX) {
        // 重试超限 → UDP 通知 APP 失败 → 结束配网
        wifi_start_udp_notify(1, ((wifi_event_sta_disconnected_t *)event_data)->reason);
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    } else {
        esp_wifi_connect();
        xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
    }
    break;
}
case WIFI_EVENT_STA_CONNECTED: {
    s_retry_num = 0;  // 连接成功，重置计数器
    break;
}
```

- 最大重试 **5 次**（由 `WIFI_RETRY_MAX` 控制）
- 超限后发送 UDP 失败通知并置位 `ESPTOUCH_DONE_BIT` 终止流程
- 连接成功后重置计数器

---

## 13 UDP 反馈协议（新增）

EspTouch ACK 只通知 APP "收到了 SSID+密码"，之后设备是否连上 WiFi、IP 是什么，APP 并不知道。
C 端新增 **UDP 广播反馈机制**，在连接成功或失败后主动通知 APP。

### 13.1 协议参数

| 参数 | 值 | 说明 |
|------|---|------|
| 目标地址 | `255.255.255.255` | 子网广播 |
| 端口 | `7001` | 由 `WIFI_NOTIFY_PORT` 定义，避免与 EspTouch ACK 端口冲突 |
| 发送次数 | 3 次 | 由 `WIFI_NOTIFY_COUNT` 定义，防止 UDP 丢包 |
| 发送间隔 | 500ms | 由 `WIFI_NOTIFY_INTERVAL_MS` 定义 |
| 数据格式 | JSON | UTF-8 编码，便于 APP 解析 |

### 13.2 JSON 数据格式

#### 成功（`status: 0`）

```json
{
  "status": 0,
  "ip": "192.168.0.3",
  "mac": "14:c1:9f:41:26:c0",
  "ssid": "上网冲浪-2.4",
  "chip": "ESP32-S3 rev0.3"
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `status` | int | `0` = 连接成功 |
| `ip` | string | ESP32 获取的 IP 地址 |
| `mac` | string | ESP32 WiFi STA MAC 地址 |
| `ssid` | string | 连接的 WiFi SSID |
| `chip` | string | ESP32 芯片型号和版本（如 `ESP32-S3 rev0.3`） |

#### 失败（`status: 1`）

```json
{
  "status": 1,
  "reason": 15,
  "mac": "14:c1:9f:41:26:c0",
  "ssid": "上网冲浪-2.4",
  "chip": "ESP32-S3 rev0.3"
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `status` | int | `1` = 连接失败 |
| `reason` | int | WiFi 断开原因码（IEEE 802.11 reason code） |
| `mac` | string | ESP32 WiFi STA MAC 地址 |
| `ssid` | string | 尝试连接的 WiFi SSID |
| `chip` | string | ESP32 芯片型号和版本（如 `ESP32-S3 rev0.3`） |

#### 用户断开（`status: 2`）

```json
{
  "status": 2,
  "reason": 0,
  "mac": "14:c1:9f:41:26:c0",
  "ssid": "上网冲浪-2.4",
  "chip": "ESP32-S3 rev0.3"
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `status` | int | `2` = 用户主动断开（`WIFI_NOTIFY_STATUS_DISCONNECT`） |
| `reason` | int | 断开原因码（用户断开时为 0） |
| `mac` | string | ESP32 WiFi STA MAC 地址 |
| `ssid` | string | 断开前的 WiFi SSID |
| `chip` | string | ESP32 芯片型号和版本（如 `ESP32-S3 rev0.3`） |

常见 reason 码：

| reason | 说明 |
|--------|------|
| 2 | 保留 |
| 15 | AP 关联到期（密码错误常见） |
| 201 | 找不到 AP |

### 13.3 发送时机

```
IP_EVENT_STA_GOT_IP
  │
  ├─ 获取 IP、MAC、芯片信息
  ├─ 组装 JSON: {"status":0, "ip":"...", "mac":"...", "ssid":"...", "chip":"ESP32-S3 rev0.3"}
  ├─ 创建 udp_notify_task
  │    ├─ socket(AF_INET, SOCK_DGRAM, 0)
  │    ├─ setsockopt(SO_BROADCAST)
  │    ├─ 循环 3 次:
  │    │   sendto(255.255.255.255:7001, json)
  │    │   vTaskDelay(500ms)
  │    └─ closesocket + vTaskDelete

WIFI_EVENT_STA_DISCONNECTED (重试 > WIFI_RETRY_MAX)
  │
  ├─ 获取 reason、ssid、MAC、芯片信息
  ├─ 组装 JSON: {"status":1, "reason":..., "mac":"...", "ssid":"...", "chip":"ESP32-S3 rev0.3"}
  ├─ 创建 udp_notify_task
  │    ├─ 同上发送流程
  │    └─ ...
  └─ 置位 ESPTOUCH_DONE_BIT → 结束配网流程
```

### 13.4 APP 侧对接

APP 在收到 EspTouch ACK 后，应继续监听 UDP 端口 `7001`：

```
手机 APP 流程：
  1. EspTouch v2 发送配网数据
  2. 收到 EspTouch ACK（SSID+密码已送达）
  3. 启动 UDP 监听，绑定端口 7001
  4. 等待 ESP32 的 UDP 广播
  5. 解析 JSON:
     - status == 0 → 配网成功，获取 IP/MAC
     - status == 1 → 配网失败，获取 reason 码
  6. 超时建议：15 秒（WiFi 连接 + DHCP 通常 < 10s）
```

APP 超时未收到 UDP 广播的可能原因：

- ESP32 连接 WiFi 后换了子网（与 APP 不在同一广播域）
- UDP 广播被路由器过滤
- WiFi 连接过程超过 15 秒

### 13.5 新增头文件

```c
#include "esp_chip_info.h"  // esp_chip_info(), esp_chip_info_t
#include "esp_mac.h"        // esp_read_mac()
#include "esp_netif.h"      // esp_netif_get_ip_info(), esp_netif_get_handle_from_ifkey()
#include "lwip/inet.h"      // htonl(), INADDR_BROADCAST
#include "lwip/sockets.h"   // socket(), sendto(), closesocket()
```

### 13.6 新增宏定义

```c
#define WIFI_NOTIFY_PORT       7001   // UDP 反馈端口
#define WIFI_NOTIFY_COUNT      3      // UDP 广播次数
#define WIFI_NOTIFY_INTERVAL_MS 500   // UDP 广播间隔(ms)
#define WIFI_RETRY_MAX         5      // WiFi 断线最大重试次数
```

---

## 14 APP 侧完整对接检查清单

### 14.1 基础对接

- [x] C 端使用 `SC_TYPE_ESPTOUCH_V2`，APP **必须**使用 EspTouch v2 协议发送
- [ ] 确认 SSID 和密码的编码方式（包长度编码）
- [ ] 如需向下兼容 v1，C 端需改为 `SC_TYPE_ESPTOUCH`（将失去 Reserved Data 能力）

### 14.2 EspTouch v2 特有对接

- [x] C 端已设置为 `SC_TYPE_ESPTOUCH_V2`，Reserved Data 功能可用
- [x] Reserved Data 最大 64 字节，通过 `esp_smartconfig_get_rvd_data()` 读取
- [ ] 当前未启用 AES 加密 → APP 发送明文即可
- [ ] 如需启用加密 → C 端和 APP 使用**完全相同**的 16 字节 AES-128 密钥

### 14.3 ACK 与完成确认

- [ ] APP 发送配网数据后等待 EspTouch UDP ACK（协议内置）
- [ ] ACK 包含 `token` 和 `cellphone_ip`
- [ ] C 端收到 `SC_EVENT_SEND_ACK_DONE` 后置位 `ESPTOUCH_DONE_BIT`
- [ ] C 端调用 `esp_smartconfig_stop()` 释放资源

### 14.4 UDP 反馈通知（新增）

- [ ] APP 收到 EspTouch ACK 后，监听 UDP 端口 `7001`
- [ ] 成功时收到 `{"status":0,"ip":"...","mac":"...","ssid":"...","chip":"ESP32-S3 rev0.3"}`
- [ ] 失败时收到 `{"status":1,"reason":...,"mac":"...","ssid":"...","chip":"ESP32-S3 rev0.3"}`
- [ ] 用户断开时收到 `{"status":2,"reason":0,"mac":"...","ssid":"...","chip":"ESP32-S3 rev0.3"}`
- [ ] APP 超时建议 15 秒
- [ ] C 端发送 3 次，间隔 500ms，防丢包

### 14.5 连接状态

- [x] C 端通过 `IP_EVENT_STA_GOT_IP` 确认成功获取 IP → 发送 UDP 成功通知
- [x] C 端 `WIFI_EVENT_STA_DISCONNECTED` 重试 5 次后发送 UDP 失败通知 → 结束配网
- [ ] APP 侧应处理各种超时场景（SmartConfig 超时、WiFi 连接超时、UDP 等待超时）

---

## 14 关键数据类型速查

### 14.1 C 端数据类型

| 类型 | 定义 | 用途 |
|------|------|------|
| `smartconfig_type_t` | `SC_TYPE_ESPTOUCH`, `SC_TYPE_AIRKISS`, `SC_TYPE_ESPTOUCH_AIRKISS`, `SC_TYPE_ESPTOUCH_V2` | 协议类型枚举 |
| `smartconfig_event_t` | `SC_EVENT_SCAN_DONE`, `SC_EVENT_FOUND_CHANNEL`, `SC_EVENT_GOT_SSID_PSWD`, `SC_EVENT_SEND_ACK_DONE` | SmartConfig 事件枚举 |
| `smartconfig_event_got_ssid_pswd_t` | 见 4.2.1 节 | 配网数据结构 |
| `smartconfig_start_config_t` | `enable_log`, `esp_touch_v2_enable_crypt`, `esp_touch_v2_key` | 启动配置 |
| `wifi_config_t` | `.sta.ssid[32]`, `.sta.password[64]`, `.sta.bssid_set`, `.sta.bssid[6]` | WiFi 配置 |
| `wifi_init_config_t` | `WIFI_INIT_CONFIG_DEFAULT()` | WiFi 初始化配置 |
| `EventGroupHandle_t` | FreeRTOS 事件组 | 状态同步 |

### 15.2 C 端常量

| 常量 | 值 | 含义 |
|------|---|------|
| `CONNECTED_BIT` | `BIT0` (0x01) | WiFi 已连接 + 获取 IP |
| `ESPTOUCH_DONE_BIT` | `BIT1` (0x02) | SmartConfig 流程完成 |
| `WIFI_NOTIFY_PORT` | `7001` | UDP 反馈端口 |
| `WIFI_NOTIFY_COUNT` | `3` | UDP 广播发送次数 |
| `WIFI_NOTIFY_INTERVAL_MS` | `500` | UDP 广播间隔(ms) |
| `WIFI_RETRY_MAX` | `5` | WiFi 断线最大重试次数 |
| `WIFI_IF_STA` | 枚举 | STA 接口 |
| `WIFI_MODE_STA` | 枚举 | STA 模式 |
| `WIFI_AUTH_WPA2_PSK` | 枚举 | WPA2-PSK 认证模式 |

---

## 15 版本与兼容性信息

| 项目 | 版本 |
|------|------|
| ESP-IDF | v5.5.4 |
| 目标芯片 | ESP32-S3 |
| SmartConfig 协议 | **EspTouch v2** (`SC_TYPE_ESPTOUCH_V2`) |
| 加密模式 | 未启用（明文），可选 AES-128 |
| Reserved Data | 支持，最多 64 字节 |
| UDP 反馈 | 端口 7001，广播 3 次间隔 500ms |
| WiFi 重试上限 | 5 次，超限发送失败通知 |
| FreeRTOS | ESP-IDF 内置版本 |
| WiFi 模式 | STA only |
| NVS 分区大小 | 24KB (0x6000) |

---

## 16 协议编码原理（APP 侧参考）

### 16.1 EspTouch 编码原理

EspTouch 利用 **802.11 MAC 帧长度的差异性**来编码数据：

1. 手机和 ESP32 必须在同一WiFi信道的无线电范围内
2. 手机将 SSID + 密码等信息分片
3. 每个分片被编码为一个特定长度的 UDP 广播包
4. ESP32 在混杂模式下接收无线帧，通过**帧长度**映射回数据字节
5. 多个分片组合还原出完整的 SSID + 密码

### 16.2 EspTouch v2 增强编码

在 v1 基础上增加：

- **Header**：指示后续数据的类型和长度
- **Reserved Data 段**：紧跟在密码之后，最大 64 字节
- **AES-128 加密**（可选）：加密 SSID + 密码 + Reserved Data 的组合明文

### 16.3 通信时序

```
时间 ──────────────────────────────────────────────────►

手机 APP:
  ├─ 广播帧1 ─┬─ 广播帧2 ─┬─ 广播帧3 ─┬─ ... ─┬─ 广播帧N ─┤
  │ (长度=编码1) │ (长度=编码2) │ (长度=编码3) │        │ (长度=编码N)│
  │             │             │             │        │             │
ESP32:         │             │             │        │             │
  ├─ 混杂模式接收 ────────────────────────────────────────────┤
  ├─ 解码 ──────────────────────────────────────────────────┤
  ├─ 得到 SSID + 密码 (+ Reserved Data) ───────────────────┤
  │                                                          │
  ├─ esp_wifi_set_config()  ────────────────────────────────┤
  ├─ esp_wifi_connect()     ────────────────────────────────┤
  │                                                          │
  ├─ UDP ACK → 手机IP ────────────────────────────────────┤
  │   (包含 token + cellphone_ip)                            │
  │                                                          │
│   ┌─ WiFi 连接成功 ─────────────────────────────────┐    │
   │   │  IP_EVENT_STA_GOT_IP                            │    │
   │   │                                                 │    │
   │   │  UDP 广播 x3 (间隔500ms) → 255.255.255.255:7001│    │
   │   │  {"status":0,"ip":"192.168.0.3",               │    │
   │   │   "mac":"14:c1:9f:41:26:c0","ssid":"xxx",      │    │
   │   │   "chip":"ESP32-S3 rev0.3"}                     │    │
   │   └─────────────────────────────────────────────────┘    │
   │                                                          │
   │   ┌─ WiFi 连接失败(重试>5次) ──────────────────────┐    │
   │   │  WIFI_EVENT_STA_DISCONNECTED                    │    │
   │   │                                                 │    │
   │   │  UDP 广播 x3 (间隔500ms) → 255.255.255.255:7001│    │
   │   │  {"status":1,"reason":15,                      │    │
   │   │   "mac":"14:c1:9f:41:26:c0","ssid":"xxx",      │    │
   │   │   "chip":"ESP32-S3 rev0.3"}                     │    │
   │   └─────────────────────────────────────────────────┘    │
```

---

## 17 调试日志格式参考

C 端运行时关键日志输出格式（当前使用 EspTouch v2 协议）：

```
I (xxxx) 【smartconfig_task】: scan done.
I (xxxx) 【smartconfig_task】: Got SSID and password
I (xxxx) 【smartconfig_task】: SSID:上网冲浪-2.4
I (xxxx) 【smartconfig_task】: PASSWORD:a123456789
I (xxxx) 【smartconfig_task】: RVD_DATA           ← EspTouch v2 必定触发
0000000000000000000000000000000000000000000000000000000000000000  ← APP未传RVD时全0
I (xxxx) 【smartconfig_task】: static ip:192.168.0.3
I (xxxx) 【smartconfig_task】: Wifi connected to AP
I (xxxx) 【smartconfig_task】: smartconfig over
```

> 注意：由于当前使用 `SC_TYPE_ESPTOUCH_V2`，`evt->type == SC_TYPE_ESPTOUCH_V2` 条件**始终为真**，
> `RVD_DATA` 日志必定输出。如果 APP 未发送 Reserved Data，hex dump 将全为 `00`。

APP 侧可通过这些日志判断：

- `scan done` → ESP32 已开始扫描，等待配网数据
- `Got SSID and password` → ESP32 已解码出 SSID 和密码
- `RVD_DATA` → v2 协议的 Reserved Data 已接收
- `Wifi connected to AP` → WiFi 连接成功
- `smartconfig over` → SmartConfig 流程结束
