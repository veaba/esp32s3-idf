# 第 7 章：模型训练

RuView 的训练管道完全用 Rust 实现（7,832 行代码，零外部 ML 依赖）。

---

## 步骤一：获取数据集

支持两个公开 WiFi CSI 数据集：

| 数据集 | 来源 | 格式 | 受试者 | 环境 | 下载 |
|--------|------|------|--------|------|------|
| MM-Fi | NeurIPS 2023 | `.npy` | 40 | 4 个房间 | [GitHub](https://github.com/ybhbingo/MMFi_dataset) |
| Wi-Pose | Entropy 2023 | `.mat` | 12 | 1 个房间 | [GitHub](https://github.com/NjtechCVLab/Wi-PoseDataset) |

下载后放到 `data/` 目录。

## 步骤二：训练

```bash
# 源码
./target/release/sensing-server --train --dataset data/ --dataset-type mmfi --epochs 100 --save-rvf model.rvf

# Docker（挂载数据目录）
docker run --rm \
  -v $(pwd)/data:/data \
  -v $(pwd)/output:/output \
  --entrypoint /app/sensing-server \
  ruvnet/wifi-densepose:latest \
  --train --dataset /data --epochs 100 --export-rvf /output/model.rvf
```

训练管道执行 10 个阶段：

1. 数据集加载（MM-Fi `.npy` 或 Wi-Pose `.mat`）
2. 硬件归一化（Intel 5300 / Atheros / ESP32 → 标准化 56 子载波）
3. 子载波重采样（114→56 或 30→56 通过 Catmull-Rom 插值）
4. 图变换器构建（17 COCO 关键点，16 骨骼边）
5. 交叉注意力训练（CSI 特征 → 人体姿态）
6. 域对抗训练（MERIDIAN：梯度反演 + 虚拟域增强）
7. 复合损失优化（MSE + CE + UV + 时间 + 骨骼 + 对称）
8. SONA 适配（micro-LoRA + EWC++）
9. 稀疏推理优化（热/冷神经元分区）
10. RVF 模型打包

## 步骤三：使用训练好的模型

```bash
./target/release/sensing-server --model model.rvf --progressive --source esp32
```

渐进式加载让 Layer A 在 <5ms 内启动基础推理，完整模型在后台加载。

## 跨环境适配（MERIDIAN）

在一个房间训练的模型在新房间通常损失 40-70% 准确率。MERIDIAN 系统（ADR-027）通过 10 秒自动校准解决此问题：

1. **部署**训练好的模型到新房间
2. **采集** ~200 帧无标记 CSI 数据（10 秒，20 Hz）
3. 系统自动通过对比测试时训练生成环境特定 LoRA 权重
4. 无标签、无重训练、无用户干预

MERIDIAN 组件（纯 Rust，+12K 参数）：

| 组件 | 功能 |
|------|------|
| 硬件归一化器 | 将任何 WiFi 芯片组重采样为标准化 56 子载波 |
| 域分解器 | 分离姿态相关特征和房间特定特征 |
| 几何编码器 | 编码 AP 位置（FiLM 条件化 + DeepSets） |
| 虚拟增强器 | 为鲁棒训练生成合成环境 |
| 快速适配 | 10 秒无监督校准（对比 TTT） |

详见 [ADR-027](../adr/ADR-027-cross-environment-domain-generalization.md)。

## CRV 信号线协议

CRV（Coordinate Remote Viewing）信号线协议（ADR-033）将 6 阶段认知感知方法论映射到 WiFi CSI 处理：

| 阶段 | CRV 术语 | WiFi 映射 |
|------|----------|-----------|
| I | 格式塔 | 去趋势自相关 → 周期性/混沌/瞬态分类 |
| II | 感官 | 6 维 CSI 特征编码（纹理、温度、亮度等） |
| III | 拓扑 | AP mesh 拓扑图与链路质量权重 |
| IV | 相干 | 相位相干性门控（接受/仅预测/拒绝/重新校准） |
| V | 审问 | 人员特定信号提取与定向子载波选择 |
| VI | 分区 | 多人分区与跨房间收敛评分 |

```bash
# 在 Cargo.toml 中启用 CRV
cargo add wifi-densepose-ruvector --features crv
```

详见 [ADR-033](../adr/ADR-033-crv-signal-line-sensing-integration.md)。

## RVF 模型容器

RuVector Format (RVF) 将训练好的模型打包为单个自包含二进制文件。

### 导出

```bash
./target/release/sensing-server --export-rvf model.rvf
```

### 加载

```bash
./target/release/sensing-server --model model.rvf --progressive
```

### 内容

RVF 文件包含：模型权重、HNSW 向量索引、量化码本、SONA 适配配置、Ed25519 训练证明、生命体征滤波参数。

### 部署目标

| 目标 | 量化 | 大小 | 加载时间 |
|------|------|------|----------|
| ESP32 / IoT | int4 | ~0.7 MB | <5ms |
| Mobile / WASM | int8 | ~6-10 MB | ~200-500ms |
| Field (WiFi-Mat) | fp16 | ~62 MB | ~2s |
| Server / Cloud | f32 | ~50+ MB | ~3s |

## 无摄像头姿态训练

无需摄像头，仅用 10 种传感器信号训练 17 关键点 COCO 姿态模型：

```bash
# 使用 Cognitum Seed（全部 10 种信号）
node scripts/train-camera-free.js \
  --data data/recordings/pretrain-*.csi.jsonl \
  --seed-url https://169.254.42.1:8443 \
  --seed-token "$SEED_TOKEN"

# 不使用 Seed（仅 CSI，3 种信号 — 仍然可用）
node scripts/train-camera-free.js \
  --data data/recordings/pretrain-*.csi.jsonl --no-seed
```

输出：82.8 KB 模型（4-bit 量化后 8 KB），17 关键点预测，0 骨骼违规。

## 摄像头监督训练（v0.7.0）

使用摄像头作为训练时的临时教师，实现 92.9% PCK@20：

```bash
# 终端 1：录制 ESP32 CSI
python scripts/record-csi-udp.py --duration 300

# 终端 2：捕获摄像头关键点
python scripts/collect-ground-truth.py --duration 300 --preview

# 对齐并训练
node scripts/align-ground-truth.js \
  --gt data/ground-truth/*.jsonl \
  --csi data/recordings/csi-*.csi.jsonl

# 训练（从 lite 开始，数据增多后升级）
node scripts/train-wiflow-supervised.js \
  --data data/paired/*.jsonl \
  --scale lite \
  --epochs 50
```

| 预设 | 参数 | 训练时间 | 适合 |
|------|------|----------|------|
| `--scale lite` | 189K | ~19 分钟 | < 1,000 样本（5 分钟采集） |
| `--scale small` | 474K | ~1 小时 | 1K-10K 样本 |
| `--scale medium` | 800K | ~2 小时 | 10K-50K 样本 |
| `--scale full` | 7.7M | ~8 小时 | 50K+ 样本（推荐 GPU） |

详见 [ADR-079](../adr/ADR-079-camera-ground-truth-training.md)。

## 下一步

- [第 8 章：自适应分类器](08-adaptive-classifier.md)
- [第 11 章：Cognitum Seed 集成](11-cognitum-seed.md)