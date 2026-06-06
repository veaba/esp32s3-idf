# RuView 项目架构

从无线电波到人体感知——完整数据流与软硬件关系。

---

## 全局架构图

```
                        ┌─────────────────────────────────────────────────────────┐
                        │                    物理世界                              │
                        │   人体运动、呼吸、心跳 → WiFi 无线电波散射             │
                        └───────────────────────┬─────────────────────────────────┘
                                                │ 无线电散射扰动
                                                ▼
┌──────────────────────────────────────────────────────────────────────────────────┐
│                              ESP32-S3 固件                                       │
│                    firmware/esp32-csi-node/                                       │
│                                                                                    │
│  Core 0 (WiFi)                Core 1 (DSP)                                        │
│  ┌─────────────────┐    ┌─────────────────────────────┐                           │
│  │ WiFi STA 连接    │    │ SPSC 环形缓冲区消费者       │                           │
│  │ CSI 回调采集     │───>│ Tier 0: 原始 I/Q 透传        │                           │
│  │ 通道跳跃 TDM     │    │ Tier 1: 相位展开 + Welford   │                           │
│  │ NDP 注入         │    │ Tier 2: 体征 + 存在 + 跌倒   │                           │
│  │                  │    │ Tier 3: WASM 模块执行        │                           │
│  └─────────────────┘    └────────────┬────────────────┘                           │
│                                      │                                            │
│  ┌───────────────────────────────────┼─────────────────────────────────┐           │
│  │  UDP 发送器 (port 5005)            │  HTTP OTA/WASM (port 8032)    │           │
│  │  Magic 0xC5110001 CSI 帧          │  Magic 0xC5110002 体征包       │           │
│  │  Magic 0xC5110004 WASM 事件       │  POST /ota, /wasm/*          │           │
│  └───────────────────────────────────┴─────────────────────────────────┘           │
└──────────────────────────────┬───────────────────────────────────────────────────┘
                               │ UDP 二进制帧
                               │ (20 字节头 + I/Q 载荷)
                               ▼
┌──────────────────────────────────────────────────────────────────────────────────┐
│                      wifi-densepose-sensing-server                                │
│                    v2/crates/wifi-densepose-sensing-server/                       │
│                                                                                    │
│  ┌────────────────┐     ┌──────────────────┐     ┌──────────────────────┐         │
│  │ UDP 监听器      │     │  模拟数据生成器   │     │ WiFi 扫描适配器       │         │
│  │ (port 5005)    │     │ (--source sim)   │     │ (netsh/iw/          │         │
│  │                 │     │                  │     │  CoreWLAN)           │         │
│  └───────┬────────┘     └────────┬─────────┘     └──────────┬──────────┘         │
│          │                       │                           │                     │
│          └───────────┬───────────┘                           │                     │
│                      ▼                                       │                     │
│  ┌───────────────────────────────────────────────────────────┴────────────────┐    │
│  │                        帧 解 析 层                                         │    │
│  │                                                                             │    │
│  │  parse_esp32_frame()     → Esp32Frame { node_id, amplitudes, phases,       │    │
│  │                              rssi, n_subcarriers, freq_mhz, ... }            │    │
│  │  parse_esp32_vitals()    → Esp32VitalsPacket { breathing_bpm, heart_bpm,   │    │
│  │                              presence, fall, n_persons, ... }                │    │
│  │  parse_wasm_output()     → 自定义 WASM 模块事件                             │    │
│  └─────────────────────────────────────────────────────────────────────────────┘    │
│                      │                                                             │
│                      ▼                                                             │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │                       每 节 点 状 态 管 理                                  │   │
│  │  HashMap<u8, NodeState> — 每个 ESP32 节点独立状态                            │   │
│  │                                                                             │   │
│  │  • frame_history: VecDeque<Vec<f64>> (最近 100 帧)                           │   │
│  │  • VitalSignDetector (FFT 呼吸/心率检测)                                    │   │
│  │  • rssi_history, coherence_score, novelty_score                             │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                      │                                                             │
│        ┌─────────────┼──────────────┬──────────────────┐                         │
│        ▼             ▼              ▼                  ▼                         │
│  ┌──────────┐  ┌──────────┐  ┌──────────────┐  ┌──────────────┐                 │
│  │ 特征提取  │  │ 平滑分类  │  │ 体征检测      │  │ 人数估计     │                 │
│  │          │  │          │  │              │  │              │                 │
│  │ 子载波   │  │ 自适应   │  │ 呼吸 FFT     │  │ 相关图       │                 │
│  │ 重要性   │  │ 基线     │  │ 0.1-0.5 Hz  │  │ min-cut     │                 │
│  │ 加权振幅 │  │ EMA 平滑 │  │ 心率 FFT     │  │ 特征值分解   │                 │
│  │ 运动频带 │  │ 滞后     │  │ 0.667-2 Hz  │  │ 场模型估计   │                 │
│  │ 呼吸频带 │  │ 去抖     │  │ 信号质量 CV   │  │              │                 │
│  │ Goertzel │  │          │  │              │  │              │                 │
│  └────┬─────┘  └────┬─────┘  └──────┬───────┘  └──────┬───────┘                 │
│       │             │               │                  │                          │
│       └──────────┬──┴───────────────┘                  │                          │
│                  ▼                                    ▼                          │
│  ┌─────────────────────────────┐  ┌──────────────────────────────────────────┐    │
│  │    跨节点融合                │  │            姿态推导                      │    │
│  │    fuse_multi_node_features()│  │  derive_pose_from_sensing()              │    │
│  │                             │  │                                          │    │
│  │ RSSI 加权平均               │  │ 17 COCO 骨架关键点                      │    │
│  │ 多节点特征融合               │  │ 每关键点调制：                          │    │
│  │ RuvSense 多基地融合         │  │   运动频带 → 身体摆动                  │    │
│  │                             │  │   呼吸频带 → 躯干起伏                  │    │
│  └────────────┬────────────────┘  │   主频 → 身体倾斜                     │    │
│               │                   │   变化点 → 四肢爆发                    │    │
│               ▼                   │ EMA 时域平滑 + 骨长约束 (±20%)        │    │
│  ┌──────────────────────────────┐ └──────────────────┬────────────────────┘    │
│  │  Kalman 姿态跟踪器            │                     │                        │
│  │  tracker_bridge::             │                     ▼                        │
│  │    tracker_update()            │  ┌──────────────────────────────────────┐   │
│  │                               │  │        信号场可视化生成               │   │
│  │  17 COCO 关键点               │  │  generate_signal_field()              │   │
│  │  6D Kalman 滤波器/点          │  │  20×20 俯视网格                      │   │
│  │  跟踪生命周期：               │  │  方差加权角度映射                     │   │
│  │  Tentative→Active→Lost→      │  │  运动强度调制                        │   │
│  │  Terminated                  │  │  呼吸环形叠加                        │   │
│  │  60% 马氏距离 + 40% 余弦     │  └──────────────────────────────────────┘   │
│  │  re-ID 任务分配              │                                             │
│  └──────────────┬───────────────┘                                             │
│                 │                                                              │
│                 ▼                                                              │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                    SensingUpdate (JSON 广播)                             │    │
│  │                                                                         │    │
│  │  {                                                                      │    │
│  │    type: "sensing_update",                                              │    │
│  │    nodes: [...],                                                        │    │
│  │    features: { mean_rssi, variance, motion_band_power,                  │    │
│  │               breathing_band_power, dominant_freq_hz,                   │    │
│  │               change_points, spectral_pwr },                            │    │
│  │    classification: { motion_level, presence, confidence },              │    │
│  │    vital_signs: { breathing_rate_bpm, heart_rate_bpm,                   │    │
│  │                   breathing_confidence, heartbeat_confidence },          │    │
│  │    persons: [ PersonDetection { id, confidence, keypoints, bbox } ],    │    │
│  │    estimated_persons, signal_field, novelty_score, pose_keypoints       │    │
│  │  }                                                                      │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                    │                              │                             │
│                    ▼                              ▼                             │
│         ┌──────────────────┐          ┌──────────────────────┐                 │
│         │ WebSocket 推送    │          │ REST API 端点         │                 │
│         │ ws://host:8765/  │          │ /health               │                 │
│         │ ws/sensing       │          │ /api/v1/sensing/latest│                 │
│         │ ws/pose          │          │ /api/v1/vital-signs    │                 │
│         └──────────────────┘          │ /api/v1/model/info    │                 │
│                                        └──────────────────────┘                 │
└──────────────────────────────────────────────────────────────────────────────────┘
                    │                                       │
                    ▼                                       ▼
         ┌──────────────────┐                   ┌──────────────────────┐
         │   浏览器 UI       │                   │  客户端应用          │
         │   ui/index.html   │                   │                      │
         │                  │                   │  wifi-densepose-cli   │
         │  • 3D 信号场     │                   │  (MAT 灾害响应命令)   │
         │  • 姿态骨架      │                   │                      │
         │  • 体征实时显示   │                   │  wifi-densepose-      │
         │  • 存在/运动指示  │                   │  desktop (Tauri 桌面) │
         │  • 设置面板      │                   │                      │
         │  • 设置引导面板  │                   │  wifi-densepose-      │
         └──────────────────┘                   │  wasm (浏览器推理)    │
                                                └──────────────────────┘
```

---

## ESP32-S3 采集了什么数据

### CSI（Channel State Information）帧

WiFi 802.11n/ac/ax 在每次数据传输时，接收端可以上报每个子载波的信道状态——即无线电波经过人体散射后的幅度和相位变化。ESP32-S3 通过 ESP-IDF CSI API 获取这些数据。

**原始数据格式（ADR-018 二进制帧）：**

```
偏移   大小   字段
0      4      Magic: 0xC5110001
4      1      节点 ID（0-255）
5      1      天线数
6      2      子载波数（LE u16，通常 56）
8      4      频率 Hz（LE u32，如 2412 = 2.4GHz 通道 1）
12     4      序列号（LE u32，单调递增）
16     1      RSSI（i8，信号强度 dBm）
17     1      噪声底（i8）
18     2      保留
20     N×2    I/Q 对（每子载波每天线 2 字节，i8 有符号）
```

单帧示例：1 天线 × 56 子载波 = 20 + 112 = 132 字节，速率 ~20 Hz/通道

**硬件限制：**
- ESP32-S3：1-2 天线，52-56 子载波（vs Intel 5300 的 3×3 MIMO + 30 子载波）
- 时钟漂移约 20-50 ppm，因此采用特征级融合而非信号级融合

### ESP32 端处理后的数据（Tier 2 体征包）

当 Edge Tier 设为 2 时，ESP32 本身执行 DSP，将 32 字节体征包发给服务器：

```
偏移   大小   字段
0      4      Magic: 0xC5110002
4      1      节点 ID
5      1      标志（bit0=存在, bit1=跌倒, bit2=运动）
6      2      呼吸率（BPM × 100，定点）
8      4      心率（BPM × 10000，定点）
12     1      RSSI（i8）
13     1      检测人数
14     2      保留
16     4      运动能量（f32）
20     4      存在评分（f32）
24     4      时间戳（ms since boot）
28     4      保留
```

速率：1 Hz，仅 32 字节/包——比原始 CSI 节省 99%+ 带宽。

### 数据来源对比

| 来源 | 命令行参数 | 数据类型 | 适用场景 |
|------|-----------|----------|----------|
| ESP32 硬件 | `--source esp32` | ADR-018 二进制帧 + 体征包 | 生产部署 |
| Windows WiFi 扫描 | `--source wifi` | BSSID RSSI 观测 | 粗略存在检测（无 CSI） |
| 模拟数据 | `--source simulated` | 合成振幅/相位 | 开发、演示、无硬件测试 |
| 自动检测 | `--source auto` | 优先 ESP32 → WiFi → 模拟 | 通用的启动方式 |

---

## 从原始 CSI 到最终输出——完整变换链

### 阶段 1：二进制解码

```
[ESP32 UDP 字节流] → parse_esp32_frame() → Esp32Frame
                                         { node_id: u8,
                                           n_subcarriers: u8 (通常56),
                                           freq_mhz: u16,
                                           rssi: i8,
                                           amplitudes: Vec<f64>,  // √(I²+Q²)
                                           phases: Vec<f64> }     // atan2(Q, I)
```

每个 I/Q 对从 2 字节有符号整数还原为复数振幅和相位。

### 阶段 2：特征提取

```
Esp32Frame + frame_history → extract_features_from_frame()
                            → FeatureInfo {
                                mean_rssi,              // 信号强度均值
                                variance,                // 振幅方差
                                motion_band_power,       // 0.5-3 Hz 运动频带功率
                                breathing_band_power,    // 0.1-0.5 Hz 呼吸频带功率
                                dominant_freq_hz,        // FFT 主频率
                                change_points,           // 变化点检测计数
                                spectral_power,          // 频谱总功率
                                subcarrier_importance,   // 子载波重要性权重
                              }
                         + ClassificationInfo {
                                motion_level,            // absent/present_still/present_active
                                presence,                // bool
                                confidence,              // 0.0-1.0
                              }
```

**关键算法：**
- **子载波重要性权重**：振幅方差大的子载波携带更多人体信息，被赋予更高权重
- **Goertzel 呼吸频率**：对 0.1-0.5 Hz 频段做定点 DFT，提取呼吸基频
- **运动频带功率**：0.5-3 Hz 频段能量，区分静止与运动

### 阶段 3：时域平滑与分类

```
FeatureInfo → smooth_and_classify_node()
            → 自适应基线学习（前 60 秒）
            → EMA 指数移动平均平滑
            → 滞后去抖（4 帧窗口，防止状态抖动）
            → 分类：absent / present_still / present_active
```

### 阶段 4：体征检测

```
每帧振幅 + 相位 → VitalSignDetector::process_frame()
                  → 1. mean_amplitude → 压入 breathing_buffer (30 秒窗口)
                    2. phase_variance → 压入 heartbeat_buffer (15 秒窗口)
                    3. FFT(breathing_buffer) → 0.1-0.5 Hz 峰值 → 呼吸率
                    4. FFT(heartbeat_buffer) → 0.667-2.0 Hz 峰值 → 心率
                    5. 振幅变异系数 → 信号质量评分
                  → VitalSigns {
                      breathing_rate_bpm: Option<f64>,
                      heart_rate_bpm: Option<f64>,
                      breathing_confidence: f64,
                      heartbeat_confidence: f64,
                      signal_quality: f64,
                    }
```

**平滑处理：** 异常值剔除（心率 ±8 BPM、呼吸 ±2 BPM）→ 截尾均值 → EMA 死区平滑

### 阶段 5：人数估计

```
frame_history → estimate_persons_from_correlation()
             → 子载波相关性图 → DynamicMinCut 分割
             → 切比 → 人数 (1/2/3)

frame_history → field_bridge::occupancy_or_fallback()
             → SVD 特征值分析（如已校准）
             → 扰动能阈值回退
```

### 阶段 6：跨节点融合（多 ESP32）

```
HashMap<u8, NodeState> → fuse_multi_node_features()
                       → RSSI 加权特征平均
                       → 活跃节点选择（10 秒内有效数据）
                       → MultistaticFuser::fuse_or_fallback()
                         → 注意力加权跨视角嵌入
                         → 几何多样性融合
```

多节点 Mesh 提供空间分集——不同位置的 ESP32 看到不同的人体散射路径，融合后显著提升定位和姿态精度。

### 阶段 7：姿态推导

```
SensingUpdate → derive_pose_from_sensing()
             → 17 COCO 骨架关键点
             → 每关键点基于以下特征调制：
                 运动频带功率 → 身体摆动幅度
                 呼吸频带功率 → 躯干起伏
                 主频率 → 身体倾斜方向
                 变化点 → 四肢动作爆发
             → 时域 EMA 平滑
             → 骨长约束（±20% 解剖学限制）
             → 相干自适应 alpha
```

**信号驱动的姿态** vs **模型推理的姿态：**

| 模式 | 标识 | 要求 | 精度 |
|------|------|------|------|
| Signal-Derived | 绿色"Signal-Derived" | 1+ ESP32，无需模型 | 存在、运动、粗略姿态 |
| Model Inference | 蓝色"Model Inference" | 4+ ESP32 + 训练好的 .rvf 模型 | 17 关键点 COCO 骨架 |

### 阶段 8：Kalman 跟踪

```
Vec<PersonDetection> → PoseTracker::tracker_update()
                     → 每关键点 6D Kalman 滤波（位置+速度）
                     → 跟踪生命周期：Tentative → Active → Lost → Terminated
                     → 60% 马氏距离 + 40% AETHER re-ID 余弦相似度
                     → 骨长约束（Lost 轨迹不再渲染）
```

### 阶段 9：信号场可视化

```
RSSI + motion_score + breathing_hz + quality + sub_variances
  → generate_signal_field()
  → 20×20 网格俯视图
  → 子载波方差 → 角度权重
  → 运动评分 → 强度调制
  → 呼吸频率 → 环形叠加
  → 归一化到 [0.0, 1.0]
```

---

## Rust 工作空间 crate 依赖关系

```
wifi-densepose-core (叶节点，无内部依赖)
    │
    ├── wifi-densepose-signal (依赖 core, ruvector-*)
    │       │
    │       └── wifi-densepose-ruvector (无内部依赖)
    │
    ├── wifi-densepose-nn (无内部依赖)
    │
    ├── wifi-densepose-wifiscan (无内部依赖)
    │
    └── wifi-densepose-vitals (无内部依赖)

wifi-densepose-mat (依赖 core, signal, nn)
    │
    ├── wifi-densepose-cli (依赖 mat)
    │
    └── wifi-densepose-sensing-server (依赖 wifiscan, mat 可选)

wifi-densepose-wasm (依赖 mat) → 浏览器推理
wifi-densepose-desktop (Tauri 桌面应用)
wifi-densepose-api (Axum REST API — 存根)
wifi-densepose-train (依赖 signal, nn) → 离线训练
wifi-densepose-pointcloud (3D 点云)
wifi-densepose-geo (地理空间)
wifi-densepose-db (数据库层)
wifi-densepose-config (配置管理)
wifi-densepose-hardware (硬件抽象)

nvsim (独立叶节点，WASM-ready)
nvsim-server (nvsim 的 Web 仪表盘)
wifi-densepose-wasm-edge (ESP32 WASM，排除在工作空间外)
ruv-neural/ (RuVector 神经子 crate 集群)
```

---

## 每个 Crate 的角色

| Crate | 角色 | 数据流位置 |
|-------|------|------------|
| `core` | 基础类型：CsiFrame, PoseEstimate, Keypoint, Confidence 等 | 全局共享词汇 |
| `signal` | CSI 信号处理：噪声去除、相位展开、特征提取、运动检测、RuvSense 多基地感知 | 阶段 2-6 |
| `nn` | 神经网络推理：ONNX/tch/Candle 三后端、ModalityTranslator、DensePoseHead | 阶段 7 (Model Inference) |
| `ruvector` | RuVector 融合：子载波分割、注意力门控、TDoA 定位、CRV 6 阶段协议 | 阶段 5-6 |
| `vitals` | 体征提取：呼吸/心率带宽滤波、Goertzel、临床异常检测 | 阶段 4 |
| `mat` | 灾害评估：检测→定位→告警三大限界上下文、Kalman 跟踪、START 分诊 | 终端应用 |
| `wifiscan` | 多 BSSID WiFi 扫描：Windows/macOS/Linux 适配器 | 替代数据源 |
| `sensing-server` | 完整服务器：UDP 接收、特征提取、体征检测、姿态跟踪、WebSocket/REST 推送 | 阶段 1-9 全链路 |
| `cli` | 命令行入口：MAT 灾害响应子命令 | 用户交互 |
| `api` | REST API 存根 | 未来的 HTTP 接口 |
| `train` | 训练管线：MmFi 数据集、子载波插值、MERIDIAN 域适应 | 离线训练 |
| `wasm` | 浏览器 WASM 推理 | 客户端推理 |
| `wasm-edge` | ESP32 WASM 感知模块（gesture/coherence/adversarial） | Tier 3 |

---

## RuvSense 六阶段流水线

多节点 ESP32 数据进入 `wifi-densepose-signal/src/ruvsense/` 后经过 6 个阶段：

```
阶段 1: 多频段融合     multiband.rs    → MultiBandCsiFrame
阶段 2: 相位对齐       phase_align.rs  → PhaseAligner
阶段 3: 多基地融合     multistatic.rs  → FusedSensingFrame
阶段 4: 相干评分       coherence.rs    → CoherenceState
阶段 5: 相干门控       coherence_gate.rs → GateDecision / GatePolicy
阶段 6: 姿态跟踪       pose_tracker.rs → PoseTrack / TrackId
```

**关键参数（RuvSenseConfig 默认值）：**

| 参数 | 默认 | 含义 |
|------|------|------|
| `max_nodes` | 4 | 最大节点数 |
| `target_hz` | 20 Hz | 目标帧率 |
| `num_channels` | 3 | 通道数（1/6/11） |
| `coherence_accept` | 0.85 | 相干接受阈值 |
| `coherence_drift` | 0.5 | 相干漂移容忍 |
| `embedding_dim` | 128 | 嵌入维度 |

---

## ESP32-S3 固件架构与 Tier 系统

### 固件双核分工

```
┌─────────────────────────────────────────────────┐
│ Core 0 (WiFi)          │ Core 1 (DSP)           │
│                         │                         │
│ WiFi STA + CSI 回调     │ SPSC 环形缓冲区消费    │
│ 通道跳跃 (ADR-029)      │ Tier 0: 原始透传        │
│ NDP 注入                │ Tier 1: 相位 + Welford  │
│ TDM 时隙管理            │ Tier 2: 体征 + 跌倒     │
│                         │ Tier 3: WASM 模块分发   │
├─────────────────────────┼─────────────────────────┤
│ NVS 配置               │ OTA 更新服务器 (8032)    │
│ 节能管理               │ UDP 发送器 (5005)        │
└─────────────────────────────────────────────────┘
```

### 处理层级对照

| Tier | ESP32 处理 | 发送内容 | 带宽 | 服务器处理 |
|------|-----------|----------|------|-----------|
| 0 | 无——透传原始 I/Q | ADR-018 帧 132+ 字节 | ~5 KB/s | 完整特征提取+体征+姿态 |
| 1 | 相位展开+Welford+Top-K+增量压缩 | 压缩帧 | ~2 KB/s | 体征检测+姿态 |
| 2 | 全部 DSP + 体征+存在+跌倒 | 32 字节体征包+压缩帧 | ~2 KB/s | 仅姿态推导+融合 |
| 3 | Tier 2 + WASM 模块执行 | 体征包+WASM 事件 | ~2 KB/s+事件 | 姿态+自定义事件 |

**关键设计原则：** ESP32 在设备端做尽可能多的处理（降低带宽、延迟、功耗），服务器侧做需要算力的融合和推理。Tier 越高，ESP32 承担越多，服务器越省力。

---

## 多节点 Mesh 感知

### 为什么需要多节点

单一 ESP32 只能看到一条 WiFi 路径上的散射变化——存在检测较好，但定位和姿态精度有限。三维空间中的人体从不同角度产生不同的散射模式，多个节点形成**多基地（multistatic）**观测阵列。

### TDM 时分协议

```
时间轴: |---slot0---|---slot1---|---slot2---|---slot0---|...
节点:      Node 0      Node 1      Node 2      Node 0
```

所有节点向同一个聚合服务器（port 5005）发送数据，通过 `node_id` 字段区分来源。

### 通道跳跃

3 通道跳跃（1/6/11）在 2.4 GHz 频段上，将子载波数从 56 扩展到 168（3×56），相当于 168 个虚拟子载波。

### 感知能力 vs 节点数

| 节点数 | 存在 | 运动 | 定位 | 呼吸 | 心率 | 多人 |
|--------|------|------|------|------|------|------|
| 1 | 良 | 良 | — | 边缘 | 差 | — |
| 3 | 优 | 优 | 良 | 良 | 边缘 | 一般 |
| 4+ | 优 | 优 | 优 | 良 | 一般 | 良 |

---

## WASM 可编程感知（Tier 3）

ESP32 从固定传感器变为可编程感知计算机。算法以 Rust 编译的 WASM 模块形式热加载：

```
Rust 源码 (gesture.rs / coherence.rs / adversarial.rs)
  │
  ▼ cargo build --target wasm32-unknown-unknown --release
WASM 二进制 (最大 128 KB)
  │
  ▼ 打包为 RVF 容器 (32B头 + 96B清单 + WASM + 64B签名)
RVF 文件
  │
  ▼ HTTP POST /wasm/upload → ESP32:8032
WASM3 解释器在 Core 1 上以 ~20 Hz 调用模块的 on_frame() 函数
  │
  ▼ 模块通过 csi_get_* API 读取传感器数据
  │
  ▼ 模块通过 csi_emit_event() 发出自定义事件
  │
  ▼ 事件打包为 Magic 0xC5110004 UDP 帧 → 服务器
```

---

## 信号处理核心算法速查

| 算法 | 位置 | 输入 | 输出 | 用途 |
|------|------|------|------|------|
| Hampel 滤波 | `signal::hampel` | 振幅序列 | 去除离群值的序列 | 噪声剔除 |
| 相位展开 | `signal::phase_sanitizer` | 相位序列 | 连续相位（无 2π 跳变） | 相位一致性 |
| Welford 统计 | 固件 `edge_processing.c` | 每帧子载波 | 增量均值和方差 | 在线统计 |
| Top-K 子载波选择 | 固件 + `signal::subcarrier_selection` | 方差向量 | 最重要的 K 个子载波索引 | 降维 |
| 双二阶 IIR 带通 | 固件 | 相位序列 | 滤波后信号 | 呼吸/心率频段分离 |
| Goertzel 算法 | `sensing-server` | 振幅序列 | 单频率 DFT 系数 | 快速呼吸频率提取 |
| FFT | `sensing-server`, `vitals` | 时间序列 | 频谱 | 心率、呼吸频谱分析 |
| DynamicMinCut | `ruvector-mincut` | 子载波相关图 | 图分割 | 人数估计 |
| SVD 特征值 | `signal::field_model` | 子载波矩阵 | 特征值谱 | 场模型人数估计 |
| Kalman 跟踪 | `sensing-server::tracker_bridge` | 姿态关键点 | 平滑轨迹 | 姿态时域平滑 |
| 注意力融合 | `ruvector::viewpoint` | 多节点特征 | 加权融合结果 | 多基地空间融合 |

---

## 部署拓扑

### 单节点（开发/演示）

```
[ESP32-S3] --UDP:5005--> [sensing-server] --WS:3001--> [浏览器]
                        [sensing-server] --HTTP:3000--> [UI]
```

### 多节点 Mesh（生产）

```
[ESP32-S3 #1] ──┐
[ESP32-S3 #2] ──┼──UDP:5005──> [sensing-server] ──HTTP/WS──> [浏览器]
[ESP32-S3 #3] ──┘               [sensing-server] ──HTTP/WS──> [Tauri 桌面应用]
                                  │
                                  ├── ESP32 #1:8032  (OTA + WASM 管理)
                                  ├── ESP32 #2:8032  (OTA + WASM 管理)
                                  └── ESP32 #3:8032  (OTA + WASM 管理)
```

### Docker 部署

```
docker compose up
  → sensing-server (Rust/Axum)
      ├─ HTTP :3000     (UI + REST API)
      ├─ WS   :3001     (实时数据流)
      └─ UDP  :5005     (ESP32 CSI 接收)
  → python-sensing (旧版 Python 后端)
      ├─ HTTP :8080     (UI)
      └─ WS   :8765     (实时数据流)
```

---

## 性能参考值

| 指标 | 数值 |
|------|------|
| ESP32 CSI 帧率 | ~20 Hz/通道 |
| 单帧带宽（Tier 0） | ~132-356 字节 |
| 体征包带宽（Tier 2） | 32 字节/秒 |
| 服务器特征提取延迟 | < 1 ms/帧 |
| 存在检测延迟 | < 1 ms（自适应基线 60 秒） |
| 呼吸率范围 | 6-30 BPM |
| 心率范围 | 40-120 BPM |
| 单节点存在检测精度 | ~100%（训练模型） |
| 4+ 节点姿态 PCK@20 | 92.9%（相机监督训练） |
| 4+ 节点姿态抖动 | < 30 mm |
| ESP32 固件体积 | ~943 KB（8MB Flash） |
| WASM 模块体积 | ~13.8 KB |
| 嵌入维度 | 128（RuvSenseConfig） |
| 帧历史深度 | 100 帧（NodeState） |
| 呼吸缓冲窗口 | 30 秒 |
| 心率缓冲窗口 | 15 秒 |