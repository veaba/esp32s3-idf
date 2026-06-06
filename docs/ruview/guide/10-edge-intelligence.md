# 第 10 章：边缘智能模块

RuView 包含 65 个 WASM 边缘智能模块，运行在 ESP32 传感器上。无需互联网、无需云费用、即时响应。

---

## 概述

每个模块是一个小型 WASM 文件（5-30 KB），通过 OTA 上传到设备。它读取 WiFi 信号数据并在 10ms 内本地做出决策。所有模块使用 `no_std` Rust，共享公共工具库，通过 12 函数 API 与主机通信。

| 类别 | 模块数 | 说明 |
|------|--------|------|
| 核心 | 7 | 手势分类、相干滤波、对抗检测、入侵检测、占用计数、体征趋势、RVF 解析 |
| 信号智能 | 6 | Flash Attention、Coherence Gate、时间压缩、稀疏恢复、人员匹配、最优传输 |
| 自适应学习 | 4 | DTW 手势学习、异常吸引子、元适配、EWC 终身学习 |
| 空间推理 | 3 | PageRank 影响、Micro HNSW、脉冲跟踪器 |
| 时间分析 | 3 | 模式序列、时间逻辑守卫、GOAP 自治 |
| AI 安全 | 2 | Prompt Shield、行为画像 |
| 量子启发 | 2 | 量子相干、干涉搜索 |
| 自治系统 | 2 | 精神符号、自愈 mesh |
| 异类数学 | 2 | 时间晶体、双曲空间 |
| 医疗健康 | 5 | 睡眠呼吸暂停、心律失常、呼吸窘迫、步态分析、癫痫检测 |
| 安防安全 | 5 | 周界入侵、武器检测、尾随、滞留、恐慌运动 |
| 智能建筑 | 5 | HVAC 存在、照明分区、电梯计数、会议室、能源审计 |
| 零售餐饮 | 5 | 队列长度、驻留热力图、客流、翻台率、货架互动 |
| 工业专项 | 5 | 叉车接近、密闭空间、洁净室、畜牧、结构振动 |
| 异域研究 | 8 | 睡眠分期、情绪检测、手语、音乐指挥、植物生长、幽灵猎人、雨检测、呼吸同步 |

## 模块详情

### 核心模块

| 模块 | 源文件 | 功能 |
|------|--------|------|
| 手势分类器 | `gesture.rs` | DTW 模板匹配手势识别 |
| 相干滤波器 | `coherence.rs` | 相位相干性门控信号质量 |
| 对抗检测器 | `adversarial.rs` | 检测物理上不可能的信号模式 |
| 入侵检测器 | `intrusion.rs` | 人与非人运动分类 |
| 占用计数器 | `occupancy.rs` | 区域级人数计数 |
| 体征趋势 | `vital_trend.rs` | 长期呼吸和心率趋势 |
| RVF 解析器 | `rvf.rs` | RVF 容器格式解析 |

### 信号智能模块

| 模块 | 源文件 | 功能 | 计算预算 |
|------|--------|------|----------|
| Flash Attention | `sig_flash_attention.rs` | 8 子载波组分块注意力 | S (<5ms) |
| Coherence Gate | `sig_coherence_gate.rs` | Z-score 相量门控含迟滞 | L (<2ms) |
| 时间压缩 | `sig_temporal_compress.rs` | 3 层自适应量化 | L (<2ms) |
| 稀疏恢复 | `sig_sparse_recovery.rs` | ISTA L1 子载波重建 | H (<10ms) |
| 人员匹配 | `sig_mincut_person_match.rs` | 匈牙利-轻量二部分配 | S (<5ms) |
| 最优传输 | `sig_optimal_transport.rs` | 切片 Wasserstein-1 距离 | L (<2ms) |

> 完整模块列表源码：[`v2/crates/wifi-densepose-wasm-edge/src/`](../../v2/crates/wifi-densepose-wasm-edge/src/)

## 计算预算分类

| 标签 | 最大延迟 | 适用场景 |
|------|----------|----------|
| L (<2ms) | 轻量 | 实时门控、压缩 |
| S (<5ms) | 标准 | 大多数模块 |
| H (<10ms) | 重量 | 稀疏恢复、异常吸引 |

## 如何使用

边缘模块通过 OTA 上传到 ESP32 节点。启用 Tier 1 或 Tier 2 边缘处理后，模块读取 CSI 信号数据并本地运行推理。

```bash
# 启用边缘 Tier 2（完整生命体征）
python firmware/esp32-csi-node/provision.py --port COM7 \
  --ssid "YourWiFi" --password "secret" --target-ip 192.168.1.20 \
  --edge-tier 2
```

## 扩展阅读

- [ADR-041](../adr/ADR-041-wasm-module-collection.md) — 边缘模块完整设计
- [Edge Modules Guide](../edge-modules/README.md) — 分类详解

## 下一步

- [第 6 章：硬件设置](06-hardware-setup.md)
- [第 11 章：Cognitum Seed 集成](11-cognitum-seed.md)