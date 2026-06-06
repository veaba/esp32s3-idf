# 第 2 章：安装与构建

本文介绍所有 RuView 安装方式。

---

## Docker 安装（推荐）

最快的方式，无需安装任何工具链：

```bash
docker pull ruvnet/wifi-densepose:latest
```

多架构镜像（amd64 + arm64），支持 Intel/AMD 和 Apple Silicon。

数据源通过 `CSI_SOURCE` 环境变量选择：

| 值 | 说明 |
|---|------|
| `auto` | （默认）检测 ESP32，未找到则回退到模拟 |
| `esp32` | 从 ESP32 接收真实 CSI 帧 |
| `simulated` | 生成合成 CSI 数据 |
| `wifi` | 主机 WiFi RSSI（容器内不可用） |

示例：

```bash
docker run -e CSI_SOURCE=esp32 -p 3000:3000 -p 5005:5005/udp ruvnet/wifi-densepose:latest
```

## 从源码构建（Rust）

### Linux 系统依赖

```bash
# Debian/Ubuntu — 桌面/Tauri 所需
sudo apt update
sudo apt install -y build-essential pkg-config \
  libglib2.0-dev libgtk-3-dev libsoup-3.0-dev \
  libjavascriptcoregtk-4.1-dev libwebkit2gtk-4.1-dev
```

### 构建

```bash
git clone https://github.com/ruvnet/RuView.git
cd RuView/v2

# 构建
cargo build --release

# 运行测试（务必使用 --no-default-features）
cargo test --workspace --no-default-features
```

> **重要**：`cargo test` 必须加 `--no-default-features`。`eigenvalue` 特性需要 BLAS（ndarray-linalg + OpenBLAS），可能不可用。

编译后的二进制文件在 `target/release/sensing-server`。

### 基准测试

```bash
cargo bench --package wifi-densepose-signal
```

| 操作 | 延迟 | 吞吐量 |
|------|------|--------|
| CSI 预处理 (4×64) | ~5.19 μs | 49-66 Melem/s |
| 相位清洗 (4×64) | ~3.84 μs | 67-85 Melem/s |
| 特征提取 (4×64) | ~9.03 μs | 7-11 Melem/s |
| 运动检测 | ~186 ns | — |
| 完整管道 | ~18.47 μs | ~54,000 fps |

## 从 crates.io 安装

所有 16 个 crate 已发布到 crates.io（v0.3.0）。可单独引入到你的 Rust 项目：

```bash
# 核心类型和 trait
cargo add wifi-densepose-core

# 信号处理（含 RuvSense 多态感知）
cargo add wifi-densepose-signal

# 神经网络推理
cargo add wifi-densepose-nn

# 灾难评估工具
cargo add wifi-densepose-mat

# ESP32 硬件 + TDM 协议 + QUIC 传输
cargo add wifi-densepose-hardware

# RuVector 集成（加 --features crv 启用 CRV 信号线协议）
cargo add wifi-densepose-ruvector --features crv

# WebAssembly 绑定
cargo add wifi-densepose-wasm

# WASM 边缘运行时（嵌入式/IoT）
cargo add wifi-densepose-wasm-edge
```

> **发布顺序**：crate 必须按依赖顺序发布。参见 [CLAUDE.md](../../CLAUDE.md#crate-publishing-order)。

## 从源码构建（Python）

Python v1 位于 `archive/v1/`：

```bash
git clone https://github.com/ruvnet/RuView.git
cd RuView

pip install -r requirements.txt
pip install -e .

# 或从 PyPI
pip install wifi-densepose
pip install wifi-densepose[gpu]   # GPU 加速
pip install wifi-densepose[all]   # 所有可选依赖
```

## 引导式安装器

交互式安装器，自动检测硬件并推荐配置：

```bash
git clone https://github.com/ruvnet/RuView.git
cd RuView
./install.sh
```

可用配置：`verify`、`python`、`rust`、`browser`、`iot`、`docker`、`field`、`full`。

非交互式：

```bash
./install.sh --profile rust --yes
```

## WASM 边缘模块构建

`wifi-densepose-wasm-edge` 针对 `wasm32-unknown-unknown`（no_std），已从工作区排除，需单独构建：

```bash
# 构建 WASM 边缘 crate
cargo build -p wifi-densepose-wasm-edge --target wasm32-unknown-unknown --release

# 独立二进制（如 ghost_hunter）
cargo build -p wifi-densepose-wasm-edge \
  --bin ghost_hunter \
  --target wasm32-unknown-unknown \
  --release \
  --no-default-features \
  --features standalone-bin
```

## 下一步

- [第 3 章：数据源配置](03-data-sources.md)
- [第 4 章：REST API](04-rest-api.md)