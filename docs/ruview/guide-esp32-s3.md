# ESP32-S3 使用指南

本指南覆盖从购买硬件到实时人体感知的完整流程。RuView 项目将 ESP32-S3 作为 WiFi CSI（Channel State Information）采集节点，通过无线电波实现穿墙人体检测、呼吸心率监测和姿态估计。

---

## 目录

1. [硬件准备](#1-硬件准备)
2. [固件构建](#2-固件构建)
3. [烧录与配网](#3-烧录与配网)
4. [启动 Sensing Server](#4-启动-sensing-server)
5. [数据流与协议](#5-数据流与协议)
6. [处理层级（Edge Tier）](#6-处理层级edge-tier)
7. [WASM 可编程感知（Tier 3）](#7-wasm-可编程感知tier-3)
8. [多节点 Mesh 组网](#8-多节点-mesh-组网)
9. [Docker 一键部署](#9-docker-一键部署)
10. [QEMU 无硬件测试](#10-qemu-无硬件测试)
11. [常见问题](#11-常见问题)
12. [相关 ADR](#12-相关-adr)

---

## 1. 硬件准备

### 最低配置（单节点）

| 组件 | 规格 | 参考价格 |
|------|------|----------|
| SoC | ESP32-S3（QFN56, 双核 240 MHz） | — |
| Flash | 8 MB（Quad SPI） | — |
| PSRAM | 8 MB（WASM 模块需要） | — |
| USB 桥 | Silicon Labs CP210x | [驱动下载](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers) |
| 推荐板 | ESP32-S3-DevKitC-1 或 XIAO ESP32-S3 | ~$7-15 |

> ESP32-C3 和原版 ESP32 **不支持**——单核性能不足以运行 CSI DSP。

### 多节点 Mesh（推荐 3-6 节点）

| 节点数 | 感知能力 |
|--------|----------|
| 1 | 存在检测、呼吸、粗略运动 |
| 2-3 | 人体定位、运动方向 |
| 4+ | 单肢体跟踪、完整姿态估计（需训练模型） |

**$54 入门套件：** 3× ESP32-S3-DevKitC-1（$10/个）+ USB 线缆（$9）+ USB 电源适配器（$15）

### 4MB 变体

4MB Flash 版本（如 ESP32-S3 SuperMini）可以使用，但**不支持显示功能**（OTA 分区更小）。使用 `sdkconfig.defaults.4mb` 构建，烧录时用 `--flash_size 4MB`。

---

## 2. 固件构建

### 方式 A：Docker 构建（推荐，跨平台）

> Windows 上必须在 Git Bash 里加 `MSYS_NO_PATHCONV=1`，否则路径会被转成 `C:/Program Files/Git/project`。ESP-IDF 在 MSYS2/Git Bash 下**完全无法工作**——`idf.py` 检测到 `$MSYSTEM` 环境变量后跳过 `main()`，即使删掉 `MSYSTEM`，`cmd.exe` 子进程也会注入 `doskey` 别名导致 ninja 链接失败。Docker 是唯一可靠的跨平台方法。

```bash
# 从仓库根目录
MSYS_NO_PATHCONV=1 docker run --rm \
  -v "$(pwd)/firmware/esp32-csi-node:/project" -w /project \
  espressif/idf:v5.2 bash -c \
  "rm -rf build sdkconfig && idf.py set-target esp32s3 && idf.py build"
```

**构建输出：**
- `build/bootloader/bootloader.bin` — 二级引导
- `build/partition_table/partition-table.bin` — 分区表
- `build/esp32-csi-node.bin` — 应用固件

### 方式 B：本地 ESP-IDF 构建（Linux/macOS）

需要 ESP-IDF v5.2+：

```bash
cd firmware/esp32-csi-node
. $HOME/esp/v5.2/esp-idf/export.sh   # 激活 ESP-IDF 环境
idf.py set-target esp32s3
idf.py build
```

### 方式 C：Windows PowerShell 本地构建

如果已安装 ESP-IDF v5.4，可使用 `build_firmware.ps1`，但需要手动修改脚本中的路径：

```powershell
# 脚本会自动清除 MSYSTEM 变量并设置 IDF 工具链路径
.\firmware\esp32-csi-node\build_firmware.ps1
```

### 4MB 变体构建

```bash
# Docker 方式——使用 4MB 配置
MSYS_NO_PATHCONV=1 docker run --rm \
  -v "$(pwd)/firmware/esp32-csi-node:/project" -w /project \
  espressif/idf:v5.2 bash -c \
  "cp sdkconfig.defaults.4mb sdkconfig.defaults && rm -rf build sdkconfig && idf.py set-target esp32s3 && idf.py build"
```

### 自定义 Kconfig

交互式菜单配置：

```bash
MSYS_NO_PATHCONV=1 docker run --rm -it \
  -v "$(pwd)/firmware/esp32-csi-node:/project" -w /project \
  espressif/idf:v5.2 bash -c \
  "idf.py set-target esp32s3 && idf.py menuconfig"
```

Kconfig 菜单：
- **CSI Node Configuration** — WiFi SSID、密码、频道、节点 ID、聚合器 IP/端口
- **Edge Intelligence (ADR-039)** — 处理层级、体征间隔、Top-K 子载波数、跌倒阈值、节能模式
- **WASM Programmable Sensing (ADR-040)** — 最大模块槽位数、签名验证开关

### 使用预编译固件（免编译）

仓库附带预编译二进制文件：

```
firmware/esp32-csi-node/release_bins/
├── bootloader.bin          # 8MB 版本的引导加载程序
├── partition-table.bin     # 8MB 版本的分区表
├── esp32-csi-node.bin      # 8MB 版本的固件
├── partition-table-4mb.bin # 4MB 版本的分区表
├── esp32-csi-node-4mb.bin  # 4MB 版本的固件
└── ota_data_initial.bin     # OTA 初始数据
```

---

## 3. 烧录与配网

### 烧录固件

找到串口号：Windows 上是 `COM7`，Linux 是 `/dev/ttyUSB0`，macOS 是 `/dev/cu.SLAB_USBtoUART`。

**8MB 版本：**

```bash
python -m esptool --chip esp32s3 --port COM7 --baud 460800 \
  write_flash --flash_mode dio --flash_size 8MB \
  0x0     firmware/esp32-csi-node/release_bins/bootloader.bin \
  0x8000  firmware/esp32-csi-node/release_bins/partition-table.bin \
  0x10000 firmware/esp32-csi-node/release_bins/esp32-csi-node.bin
```

**4MB 版本：**

```bash
python -m esptool --chip esp32s3 --port COM7 --baud 460800 \
  write_flash --flash_mode dio --flash_size 4MB \
  0x0     firmware/esp32-csi-node/release_bins/bootloader.bin \
  0x8000  firmware/esp32-csi-node/release_bins/partition-table-4mb.bin \
  0x10000 firmware/esp32-csi-node/release_bins/esp32-csi-node-4mb.bin
```

> `flash_mode dio` 是必需的——ESP32-S3 默认使用 DIO 模式，使用 QIO 会导致启动失败。

### 配网（无需重编译固件）

烧录后，通过 NVS 分区写入 WiFi 凭据和聚合器地址：

```bash
# 最小配网——WiFi + 目标 IP
python scripts/provision.py --port COM7 \
  --ssid "YourWiFi" --password "YourPassword" \
  --target-ip 192.168.1.20

# 完整配网——包括处理层级、体征参数、WASM 设置
python scripts/provision.py --port COM7 \
  --ssid "MyWiFi" --password "secret" \
  --target-ip 192.168.1.20 --target-port 5005 \
  --node-id 1 --edge-tier 2 \
  --pres-thresh 0.05 --fall-thresh 5.0 \
  --vital-window 256 --vital-interval 1000 \
  --subk-count 8 --wasm-verify
```

**NVS 配置参数参考：**

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `--ssid` | string | `wifi-densepose` | WiFi 名称 |
| `--password` | string | *(空)* | WiFi 密码 |
| `--target-ip` | string | `192.168.1.100` | 聚合服务器 IP |
| `--target-port` | u16 | `5005` | 聚合服务器 UDP 端口 |
| `--node-id` | u8 | `1` | 节点 ID（0-255） |
| `--edge-tier` | u8 | `2` | 处理层级：0=原始, 1=基础DSP, 2=完整流水线 |
| `--pres-thresh` | float | 0（自动） | 存在检测阈值（0=60秒自学习） |
| `--fall-thresh` | float | `2.0` rad/s² | 跌倒检测阈值 |
| `--vital-window` | u16 | `256` | 相位历史窗口深度（32-256） |
| `--vital-interval` | u16 | `1000` ms | 体征包发送间隔 |
| `--subk-count` | u8 | `8` | Top-K 子载波数（1-32） |
| `--wasm-verify` | flag | 禁用 | 启用 Ed25519 签名验证 |
| `--no-wasm-verify` | flag | — | 禁用签名验证（开发用） |
| `--wasm-pubkey` | hex(64) | — | Ed25519 公钥（64个十六进制字符） |

### 串口监视

```bash
python -m serial.tools.miniterm COM7 115200
```

正常启动输出：

```
I (321) main: ESP32-S3 CSI Node (ADR-018) -- Node ID: 1
I (345) main: WiFi STA initialized, connecting to SSID: wifi-densepose
I (1023) main: Connected to WiFi
I (1025) main: CSI streaming active -> 192.168.1.100:5005 (edge_tier=2, OTA=ready, WASM=ready)
```

---

## 4. 启动 Sensing Server

Sensing Server 是 RuView 的核心主机端程序，接收 ESP32 CSI 数据、运行信号处理算法，并通过 WebSocket 推送到浏览器。

### 方式 A：从源码构建运行

```bash
cd v2
cargo build -p wifi-densepose-sensing-server --no-default-features

# 自动检测数据源（优先 ESP32，回退模拟数据）
./target/debug/sensing-server --http-port 3000 --source auto

# 强制使用 ESP32 实时数据
./target/debug/sensing-server --http-port 3000 --source esp32

# 强制使用模拟数据（无需硬件）
./target/debug/sensing-server --http-port 3000 --source simulated

# 加载训练好的模型进行姿态估计
./target/debug/sensing-server --http-port 3000 --source esp32 --model path/to/model.rvf

# 多节点 Mesh——指定节点几何位置
./target/debug/sensing-server --http-port 3000 --source esp32 \
  --node-positions "0.0,0.0,0.0;3.0,0.0,0.0;0.0,4.0,0.0"
```

### 方式 B：Docker Compose

```bash
cd docker

# 自动模式——检测 ESP32，无则回退模拟数据
docker compose up

# 强制 ESP32 实时数据
CSI_SOURCE=esp32 docker compose up

# 仅模拟数据
CSI_SOURCE=simulated docker compose up
```

### Sensing Server 命令行参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `--http-port` | `8080` | HTTP UI/REST API 端口 |
| `--ws-port` | `8765` | WebSocket 端口 |
| `--udp-port` | `5005` | ESP32 CSI UDP 接收端口 |
| `--ui-path` | `../../ui` | UI 静态文件路径 |
| `--tick-ms` | `100` | 数据推送间隔（ms） |
| `--bind-addr` | `127.0.0.1` | 绑定地址（`0.0.0.0` 允许外部访问） |
| `--source` | `auto` | 数据源：`auto`/`esp32`/`wifi`/`simulated` |
| `--model PATH` | — | 加载训练好的 `.rvf` 模型 |
| `--calibrate` | — | 启动时空场模型校准 |
| `--node-positions` | — | 多节点几何位置 `"x,y,z;x,y,z;..."` |
| `--benchmark` | — | 运行 1000 帧体征基准测试后退出 |

### 打开 UI

浏览器访问 http://localhost:3000

主机端网络配置：

```powershell
# Windows——添加辅助 IP（让 ESP32 能访问服务器）
New-NetIPAddress -IPAddress 192.168.1.100 -PrefixLength 24 -InterfaceAlias "Wi-Fi"

# Windows 防火墙——放行 UDP 5005
netsh advfirewall firewall add rule name="ESP32 CSI" dir=in action=allow protocol=UDP localport=5005
```

```bash
# Linux
sudo ip addr add 192.168.1.100/24 dev wlan0
sudo ufw allow 5005/udp
```

---

## 5. 数据流与协议

### 端到端数据流

```
ESP32-S3 节点                            主机
+----------------------------------+     +---------------------------+
| Core 0 (WiFi)  | Core 1 (DSP)    |     |                           |
|                 |                  |     |                           |
| WiFi STA -------> SPSC Ring Buffer |     |                           |
| CSI Callback     |                  |     |                           |
| Channel Hop      v                  |     |                           |
| NDP Inject    +--Tier 0: 原始 ----------> | UDP:5005                 |
|               |  Tier 1: 压缩 ------+---->| Sensing Server (Rust)    |
|               |  Tier 2: 体征 -------+->|                           |
|               |  Tier 3: WASM --------+>|                           |
|               +                    |     |     |                     |
| NVS 配置     OTA/WASM HTTP :8032  |     |     v                     |
| 节能管理     POST /ota            |     |   Web UI (:3000)          |
|              POST /wasm/upload     |     |   姿态 + 体征 + 告警      |
+----------------------------------+     +---------------------------+
```

### 网络端口

| 端口 | 协议 | 方向 | 用途 |
|------|------|------|------|
| 5005 | UDP | ESP32 → 服务器 | CSI 数据帧 |
| 3000 | TCP | 服务器 → 浏览器 | HTTP UI + REST API |
| 8765 | TCP | 服务器 → 浏览器 | WebSocket 实时流 |
| 8032 | TCP | 浏览器 → ESP32 | OTA 固件更新 + WASM 模块管理 |

### ADR-018 二进制帧格式

**CSI 帧（Magic `0xC5110001`）—— Tier 0：**

```
偏移   大小   字段
0      4      Magic: 0xC5110001
4      1      Node ID
5      1      天线数
6      2      子载波数（LE u16）
8      4      频率 Hz（LE u32, 如 2412 = 2.4GHz 通道 1）
12     4      序列号（LE u32）
16     1      RSSI（i8）
17     1      噪声底（i8）
18     2      保留
20     N*2    I/Q 对（n_antennas × n_subcarriers × 2 bytes）
```

示例：3 天线 × 56 子载波 = 20 + 336 = 356 字节/帧

**体征包（Magic `0xC5110002`）—— Tier 2：**

```
偏移   大小   字段
0      4      Magic: 0xC5110002
4      1      Node ID
5      1      标志位（bit0=存在, bit1=跌倒, bit2=运动）
6      2      呼吸率（BPM × 100, 定点数）
8      4      心率（BPM × 10000, 定点数）
12     1      RSSI（i8）
13     1      检测人数
14     2      保留
16     4      运动能量（f32）
20     4      存在评分（f32）
24     4      时间戳（ms since boot）
28     4      保留
```

速率：1 Hz，32 字节/包

**WASM 事件（Magic `0xC5110004`）—— Tier 3：**

```
偏移   大小   字段
0      4      Magic: 0xC5110004
4      1      Node ID
5      1      Module ID
6      2      事件数
8     N×5    事件数组（u8 类型 + f32 值）
```

---

## 6. 处理层级（Edge Tier）

Edge Tier 决定 ESP32 在设备端处理多少数据，通过 NVS `edge_tier` 键或 Kconfig `CONFIG_CSI_EDGE_TIER` 配置，**无需重编译固件**：

| Tier | 名称 | 处理内容 | 输出 | 带宽 |
|------|------|----------|------|------|
| 0 | 原始透传 | 直接转发 I/Q 数据 | ADR-018 帧 | ~5 KB/s |
| 1 | 基础 DSP | 相位展开 + Welford 统计 + Top-K 选择 + 增量压缩 | 压缩帧 | ~2 KB/s（70% 压缩） |
| 2 | 完整流水线 | Tier 1 + 双二阶 IIR 带通滤波 + 过零 BPM + 存在/跌倒检测 | 体征包 + 压缩帧 | ~2 KB/s |
| 3 | WASM 可编程 | Tier 2 + 用户上传的 WASM 模块 | 体征包 + 压缩帧 + 事件 | ~2 KB/s + 事件 |

切换 Tier——只用 `provision.py` 重写 NVS：

```bash
# 切换到原始透传模式
python scripts/provision.py --port COM7 --edge-tier 0

# 切换到完整流水线模式
python scripts/provision.py --port COM7 --edge-tier 2
```

### Tier 2 体征检测性能

| 指标 | 数值 |
|------|------|
| 呼吸率 | 6-30 BPM |
| 心率 | 40-120 BPM |
| 存在检测延迟 | < 1 ms |
| 自校准时间 | 60 秒（前 60 秒学习环境基线） |
| 多人估计 | 最多 4 人（子载波聚类） |

> **注意：** 默认跌倒阈值（2.0 rad/s²）过于敏感，部署环境建议使用 `--fall-thresh 5.0` 到 `8.0`。

---

## 7. WASM 可编程感知（Tier 3）

Tier 3 允许在 ESP32 上热加载 Rust 编译的 WASM 感知模块，无需重刷固件。

### 构建 WASM 模块

```bash
cd v2

# 构建所有默认模块（gesture, coherence, adversarial）
cargo build -p wifi-densepose-wasm-edge --target wasm32-unknown-unknown --release

# 构建独立 ghost_hunter 二进制
cargo build -p wifi-densepose-wasm-edge --bin ghost_hunter \
  --target wasm32-unknown-unknown --release \
  --no-default-features --features standalone-bin
```

### 上传 WASM 模块到 ESP32

```bash
# 上传模块
curl -X POST http://<ESP32_IP>:8032/wasm/upload --data-binary @gesture.rvf

# 列出已加载模块
curl http://<ESP32_IP>:8032/wasm/list

# 启动模块
curl -X POST http://<ESP32_IP>:8032/wasm/start/0

# 停止模块
curl -X POST http://<ESP32_IP>:8032/wasm/stop/0

# 删除模块
curl -X DELETE http://<ESP32_IP>:8032/wasm/0
```

### RVF 容器格式

WASM 模块打包为 RVF（RuVector Format）签名容器：

```
+------------------+-------------------+------------------+------------------+
| 头部 (32 B)      | 清单 (96 B)       | WASM 载荷        | Ed25519 签名 (64B)|
+------------------+-------------------+------------------+------------------+
```

| 字段 | 大小 | 内容 |
|------|------|------|
| 头部 | 32 字节 | Magic (`RVF\x01`)、格式版本、段大小、标志 |
| 清单 | 96 字节 | 模块名、作者、能力位掩码、预算请求、SHA-256 构建哈希 |
| WASM 载荷 | 可变 | 编译后的 `.wasm` 二进制（最大 128 KB） |
| 签名 | 64 字节 | Ed25519 签名（覆盖头部 + 清单 + WASM） |

### WASM Host API

模块可导入以下 `csi` 命名空间的函数：

| 函数 | 签名 | 说明 |
|------|------|------|
| `csi_get_phase` | `(i32) -> f32` | 指定子载波的相位（弧度） |
| `csi_get_amplitude` | `(i32) -> f32` | 指定子载波的幅度 |
| `csi_get_variance` | `(i32) -> f32` | 指定子载波的 Welford 方差 |
| `csi_get_bpm_breathing` | `() -> f32` | Tier 2 呼吸率 BPM |
| `csi_get_bpm_heartrate` | `() -> f32` | Tier 2 心率 BPM |
| `csi_get_presence` | `() -> i32` | 存在标志（0=无人, 1=有人） |
| `csi_get_motion_energy` | `() -> f32` | 运动能量标量 |
| `csi_get_n_persons` | `() -> i32` | 检测到的人数 |
| `csi_get_timestamp` | `() -> i32` | 启动后毫秒数 |
| `csi_emit_event` | `(i32, f32)` | 发射自定义事件（类型, 值） |
| `csi_log` | `(i32, i32)` | 调试日志（指针, 长度） |
| `csi_get_phase_history` | `(i32, i32) -> i32` | 复制相位环形缓冲到 WASM 内存 |

### WASM 模块生命周期

每个模块必须导出 3 个函数：

| 导出 | 调用时机 | 用途 |
|------|----------|------|
| `on_init()` | 模块启动时调用一次 | 分配状态、初始化算法 |
| `on_frame(n_subcarriers: i32)` | 每帧 CSI 数据（~20 Hz） | 处理传感器数据、发射事件 |
| `on_timer()` | 可配置定时间隔（默认 1s） | 周期性聚合输出 |

### 内置模块

| 模块 | 文件 | 说明 |
|------|------|------|
| **gesture** | `gesture.rs` | DTW 模板匹配——挥手、推、拉、滑动 |
| **coherence** | `coherence.rs` | 相位相干性监测（迟滞门限） |
| **adversarial** | `adversarial.rs` | 信号异常检测——相位跳变、平坦线、能量尖峰 |

### 安全机制

| 保护 | 详情 |
|------|------|
| 内存隔离 | 每槽固定 160 KB PSRAM（无堆碎片） |
| 预算守卫 | 每帧 10 ms 上限，连续 10 次超限自动停止 |
| 签名验证 | Ed25519 默认开启，开发时可用 `--no-wasm-verify` 关闭 |
| 哈希校验 | WASM 载荷 SHA-256 与清单对比 |
| 槽位限制 | 最多 4 并发模块（可配置到 8） |

---

## 8. 多节点 Mesh 组网

### TDM 时分协议（ADR-029）

多节点部署时，节点通过 TDM（Time Division Multiplexing）协议轮流发送 CSI 帧，避免冲突。

```
时间轴: |---slot0---|---slot1---|---slot2---|---slot0---|...
节点:      Node 0      Node 1      Node 2      Node 0
```

NVS 配置：

```bash
# 节点 1（TDM 槽位 0，总共 3 个节点）
python scripts/provision.py --port COM7 \
  --node-id 1 --tdm-slot 0 --tdm-nodes 3 \
  --ssid "WiFi" --password "pass" --target-ip 192.168.1.100

# 节点 2（TDM 槽位 1，总共 3 个节点）
python scripts/provision.py --port COM8 \
  --node-id 2 --tdm-slot 1 --tdm-nodes 3 \
  --ssid "WiFi" --password "pass" --target-ip 192.168.1.100

# 节点 3（TDM 槽位 2，总共 3 个节点）
python scripts/provision.py --port COM9 \
  --node-id 3 --tdm-slot 2 --tdm-nodes 3 \
  --ssid "WiFi" --password "pass" --target-ip 192.168.1.100
```

### 通道跳跃（Channel Hopping）

多频段感知使用 WiFi 通道 1、6、11（2.4 GHz）：

```bash
# 在通道 1、6、11 之间跳跃，每个通道驻留 50ms
python scripts/provision.py --port COM7 \
  --chan-list "1,6,11" --dwell-ms 50 \
  --hop-count 3
```

### 多节点效能对照

| 节点数 | 存在检测 | 粗略运动 | 房间定位 | 呼吸 | 心率 | 多人计数 |
|--------|----------|----------|----------|------|------|----------|
| 1 | 良 | 良 | — | 边缘 | 差 | — |
| 3 | 优 | 优 | 良 | 良 | 边缘 | 一般 |
| 6 | 优 | 优 | 优 | 良 | 一般 | 良 |

---

## 9. Docker 一键部署

### 自动检测模式

```bash
cd docker

# 自动检测 ESP32（UDP:5005），无硬件则回退模拟数据
docker compose up
```

### 强制 ESP32 模式

```bash
# 确保主机能收到 ESP32 的 UDP 帧
CSI_SOURCE=esp32 docker compose up

# 挂载训练好的模型
docker compose run -v /path/to/models:/app/models -e MODELS_DIR=/app/models sensing-server
```

### 仅模拟数据

```bash
CSI_SOURCE=simulated docker compose up
```

### 端口映射

| 端口 | 协议 | 服务 | 用途 |
|------|------|------|------|
| 3000 | TCP | sensing-server | REST API + UI |
| 3001 | TCP | sensing-server | WebSocket |
| 5005 | UDP | sensing-server | ESP32 CSI 帧 |
| 8080 | TCP | python-sensing | 旧版 Python UI |
| 8765 | TCP | python-sensing | 旧版 WebSocket |

---

## 10. QEMU 无硬件测试

无需 ESP32 硬件，使用 QEMU 模拟运行固件。

### 安装 QEMU（Espressif 分支）

```bash
git clone --depth 1 https://github.com/espressif/qemu.git /tmp/qemu
cd /tmp/qemu
./configure --target-list=xtensa-softmmu --enable-slirp
make -j$(nproc)
sudo cp build/qemu-system-xtensa /usr/local/bin/
```

### 构建并运行

```bash
cd firmware/esp32-csi-node

# 1. 构建（启用 mock CSI）
idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.qemu" build

# 2. 创建合并闪存镜像
esptool.py --chip esp32s3 merge_bin -o build/qemu_flash.bin \
  --flash_mode dio --flash_freq 80m --flash_size 8MB \
  0x0     build/bootloader/bootloader.bin \
  0x8000  build/partition_table/partition-table.bin \
  0x20000 build/esp32-csi-node.bin

# 3. 在 QEMU 中运行
qemu-system-xtensa -machine esp32s3 -nographic \
  -drive file=build/qemu_flash.bin,if=mtd,format=raw \
  -serial mon:stdio -no-reboot
```

### Mock CSI 场景

固件自动循环 10 个测试场景：

| 场景 | 持续 | 预期输出 |
|------|------|----------|
| 空房间 | 10s | `presence=0` |
| 静止人员 | 10s | `presence=1`, 呼吸率 10-25 BPM |
| 走动人员 | 10s | `presence=1`, 运动能量 > 0.5 |
| 跌倒事件 | 5s | `fall=1` |
| 多人 | 15s | `n_persons=2` |
| 通道扫描 | 5s | 通道 1→6→11 |
| MAC 过滤 | 5s | 丢弃错误 MAC 帧 |
| 缓冲区溢出 | 3s | 1000 帧突发不崩溃 |
| 边界 RSSI | 5s | RSSI 扫描 -127 到 0 |
| 零长度帧 | 2s | `iq_len=0` 不崩溃 |

### GDB 调试

```bash
# 启动 QEMU（暂停，GDB 端口 1234）
qemu-system-xtensa -machine esp32s3 -nographic \
  -drive file=build/qemu_flash.bin,if=mtd,format=raw \
  -serial mon:stdio -s -S

# 另一终端连接 GDB
xtensa-esp-elf-gdb build/esp32-csi-node.elf \
  -ex "target remote :1234" \
  -ex "b edge_processing.c:dsp_task" \
  -ex "b csi_collector.c:csi_serialize_frame" \
  -ex "continue"
```

### 代码覆盖率

```bash
idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.qemu;sdkconfig.coverage" build
# 运行 QEMU 后：
lcov --capture --directory build --output-file coverage.info
lcov --remove coverage.info '*/esp-idf/*' '*/test/*' --output-file coverage_filtered.info
genhtml coverage_filtered.info --output-directory build/coverage_report
```

---

## 11. 常见问题

| 症状 | 原因 | 解决方案 |
|------|------|----------|
| 无串口输出 | 波特率错误 | 使用 `115200` |
| WiFi 连不上 | SSID/密码错误 | 重跑 `provision.py` 修正凭据 |
| 收不到 UDP 帧 | 防火墙阻挡 | `netsh advfirewall firewall add rule name="ESP32 CSI" dir=in action=allow protocol=UDP localport=5005` |
| `idf.py` 在 Windows 失败 | Git Bash/MSYS2 不兼容 | 使用 Docker——这是 Windows 上唯一可靠的构建方式 |
| CSI 回调不触发 | 混杂模式问题 | 检查 `csi_collector.c` 中 `esp_wifi_set_promiscuous(true)` |
| WASM 上传被拒 | 签名验证 | 开发时 `--no-wasm-verify` 关闭签名验证 |
| 体征读数不稳定 | 校准期 | 等待 60 秒让自适应阈值学习环境基线 |
| OTA 更新失败 | 固件太大 | 确保二进制 < 1 MB（当前余量约 6%） |
| Docker 路径错误（Windows） | MSYS 路径转换 | 命令前缀加 `MSYS_NO_PATHCONV=1` |
| ESP32-S3 启动失败 | Flash 模式错误 | 烧录时必须用 `--flash_mode dio` |
| 4MB 变体空间不足 | 显示代码占空间 | 4MB 版本不含显示支持，确保用 `sdkconfig.defaults.4mb` 构建 |

---

## 12. 相关 ADR

| ADR | 标题 | 状态 |
|-----|------|------|
| [ADR-012](docs/adr/ADR-012-esp32-csi-sensor-mesh.md) | ESP32 CSI 传感器 Mesh | Accepted |
| [ADR-018](docs/adr/ADR-018-esp32-dev-implementation.md) | ESP32 开发实现 | Accepted |
| [ADR-029](docs/adr/ADR-029-ruvsense-multistatic-sensing-mode.md) | 多频段 Mesh 感知模式 | Accepted |
| [ADR-039](docs/adr/ADR-039-esp32-edge-intelligence.md) | 边缘智能层级 0-2 | Accepted |
| [ADR-040](docs/adr/ADR-040-wasm-programmable-sensing.md) | WASM 可编程感知 | Alpha |
| [ADR-057](docs/adr/ADR-057-build-time-csi-guard.md) | 编译时 CSI 守卫 | Accepted |
| [ADR-059](docs/adr/ADR-059-live-esp32-csi-pipeline.md) | 实时 ESP32 CSI 流水线 | Accepted |
| [ADR-060](docs/adr/ADR-060-provision-channel-mac-filter.md) | 配网 - 通道与 MAC 过滤 | Accepted |
| [ADR-061](docs/adr/ADR-061-qemu-esp32s3-firmware-testing.md) | QEMU ESP32-S3 固件测试 | Proposed |