# π RuView 使用教程

本目录包含 RuView（WiFi-DensePose）项目的完整使用教程，按章节组织。

## 章节目录

| 章节 | 文件 | 内容 |
|------|------|------|
| 第 1 章 | [01-quick-start.md](01-quick-start.md) | 快速入门：30 秒演示、管道验证 |
| 第 2 章 | [02-installation.md](02-installation.md) | 安装与构建：Docker、源码、crates.io、Python |
| 第 3 章 | [03-data-sources.md](03-data-sources.md) | 数据源配置：模拟、WiFi、ESP32、Cognitum Seed |
| 第 4 章 | [04-rest-api.md](04-rest-api.md) | REST API 参考：全部端点详解与示例 |
| 第 5 章 | [05-websocket.md](05-websocket.md) | WebSocket 实时流：Python/JS/curl 示例 |
| 第 6 章 | [06-hardware-setup.md](06-hardware-setup.md) | ESP32 硬件设置：固件刷写、配置、多节点组网 |
| 第 7 章 | [07-model-training.md](07-model-training.md) | 模型训练：数据集、训练流程、MERIDIAN 跨环境适配 |
| 第 8 章 | [08-adaptive-classifier.md](08-adaptive-classifier.md) | 自适应分类器：录制、训练、在线推理 |
| 第 9 章 | [09-vital-signs.md](09-vital-signs.md) | 生命体征检测：呼吸率、心率、信号平滑 |
| 第 10 章 | [10-edge-intelligence.md](10-edge-intelligence.md) | 边缘智能模块：65 个 WASM 模块详解 |
| 第 11 章 | [11-cognitum-seed.md](11-cognitum-seed.md) | Cognitum Seed 集成：向量存储、见证链、MCP 代理 |
| 第 12 章 | [12-pointcloud.md](12-pointcloud.md) | 3D 点云：摄像头+WiFi CSI 融合、训练与部署 |
| 第 13 章 | [13-troubleshooting.md](13-troubleshooting.md) | 故障排除：常见问题与解决方案 |
| 第 14 章 | [14-faq.md](14-faq.md) | 常见问题解答 |

## 前置条件

| 需求 | 最低配置 | 推荐配置 |
|------|---------|---------|
| 操作系统 | Windows 10/11, macOS 10.15, Ubuntu 18.04 | 最新稳定版 |
| 内存 | 4 GB | 8 GB+ |
| 磁盘 | 2 GB 可用 | 5 GB 可用 |
| Docker（Docker 方式） | Docker 20+ | Docker 24+ |
| Rust（源码构建） | 1.70+ | 1.85+ |
| Python（Python 方式） | 3.10+ | 3.13+ |

## 硬件选项

| 选项 | 硬件 | 费用 | 完整 CSI | 能力 |
|------|------|------|----------|------|
| ESP32 + Cognitum Seed（推荐） | ESP32-S3 + Cognitum Seed | ~$140 | 是 | 姿态、呼吸、心跳、运动、存在 + 持久向量存储 |
| ESP32 Mesh | 3-6× ESP32-S3 + WiFi 路由器 | ~$54 | 是 | 姿态、呼吸、心跳、运动、存在 |
| 研究 NIC | Intel 5300 / Atheros AR9580 | ~$50-100 | 是 | 3×3 MIMO 完整 CSI |
| 任意 WiFi | Windows/macOS/Linux 笔记本 | $0 | 否 | 仅 RSSI：粗略存在和运动检测 |

无硬件？使用模拟数据验证管道：`python archive/v1/data/proof/verify.py`