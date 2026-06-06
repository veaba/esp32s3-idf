# 第 11 章：Cognitum Seed 集成

Cognitum Seed 是一个 Pi Zero 2 W 附件（约 $15），为 ESP32-S3 添加持久向量存储、kNN 相似性搜索、加密见证链和 AI 可访问感知。

---

## Seed 添加的能力

| 能力 | 说明 |
|------|------|
| RVF 向量存储 | 持久化 8 维特征向量，内容寻址 ID + kNN 搜索（余弦、L2、点积） |
| 见证链 | SHA-256 防篡改审计轨迹 |
| Ed25519 认证 | 设备绑定密钥对，感知数据加密认证 |
| 传感器融合 | BME280（温湿度气压）、PIR 运动、干簧开关、4 通道 ADC 提供环境真实值 |
| MCP 代理 | 114 个 JSON-RPC 2.0 工具，AI 助手可直接查询感知状态 |
| 反射规则 | 基于脆弱性、漂移、异常阈值的自动告警触发 |

## 设置步骤

### 1. 插入 Cognitum Seed

通过 USB 插入，出现为网络适配器（169.254.42.1）。

### 2. 配对客户端

```bash
# 打开 30 秒配对窗口（仅 USB 安全）
curl -sk -X POST https://169.254.42.1:8443/api/v1/pair/window

# 配对
curl -sk -X POST https://169.254.42.1:8443/api/v1/pair \
  -H 'Content-Type: application/json' -d '{"client_name":"my-laptop"}'
# 保存返回的 token — 只显示一次
```

### 3. 预置 ESP32

```bash
python firmware/esp32-csi-node/provision.py --port COM9 \
  --ssid "YourWiFi" --password "secret" \
  --target-ip 192.168.1.20 --target-port 5006 --node-id 1
```

### 4. 运行桥接

```bash
export SEED_TOKEN="your-pairing-token"
python scripts/seed_csi_bridge.py \
  --seed-url https://169.254.42.1:8443 --token "$SEED_TOKEN" \
  --udp-port 5006 --batch-size 10 --validate
```

桥接接收 ESP32 UDP 数据并通过 HTTPS 摄入 Seed。

### 5. 检查 Seed 状态

```bash
python scripts/seed_csi_bridge.py --token "$SEED_TOKEN" --stats
```

### 6. 触发压缩

```bash
python scripts/seed_csi_bridge.py --token "$SEED_TOKEN" --compact
```

## 特征向量维度

特征向量（magic `0xC5110003`，48 字节，1 Hz）：

| 维度 | 特征 | 范围 | 来源 |
|------|------|------|------|
| 0 | 存在评分 | 0.0-1.0 | `s_presence_score / 10.0` |
| 1 | 运动能量 | 0.0-1.0 | `s_motion_energy / 10.0` |
| 2 | 呼吸率 | 0.0-1.0 | `s_breathing_bpm / 30.0` |
| 3 | 心率 | 0.0-1.0 | `s_heartrate_bpm / 120.0` |
| 4 | 相位方差 | 0.0-1.0 | Top-K 子载波 Welford 方差均值 |
| 5 | 人数 | 0.0-1.0 | 活跃人数 / 4 |
| 6 | 跌倒检测 | 0.0 或 1.0 | 二值跌倒标志 |
| 7 | RSSI | 0.0-1.0 | `(rssi + 100) / 100` |

## 架构

```
ESP32-S3 ($9)  ──UDP:5006──>  Host (bridge)  ──HTTPS──>  Cognitum Seed ($15)
  CSI @ 100 Hz                seed_csi_bridge.py           RVF 向量存储
  特征 @ 1 Hz               批量、验证                      kNN 图 + 边界
  生命体征 @ 1 Hz            NaN 拒绝                       见证链
                              源 IP 过滤                     114 工具 MCP 代理
```

## 自学习 WiFi AI

Cognitum Seed 支持自学习 WiFi AI（ADR-024）：

```bash
# 1. 从原始 WiFi 数据自监督学习（无需标签）
cargo run -p wifi-densepose-sensing-server -- --pretrain --dataset data/csi/ --pretrain-epochs 50

# 2. 用姿态标签微调获得完整能力
cargo run -p wifi-densepose-sensing-server -- --train --dataset data/mmfi/ --epochs 100 --save-rvf model.rvf

# 3. 使用模型 — 从实时 WiFi 提取指纹
cargo run -p wifi-densepose-sensing-server -- --model model.rvf --embed

# 4. 搜索 — 查找相似环境或检测异常
cargo run -p wifi-densepose-sensing-server -- --model model.rvf --build-index env
```

详见 [ADR-069](../adr/ADR-069-cognitum-seed-csi-pipeline.md) 和 [ADR-024](../adr/ADR-024-contrastive-csi-embedding-model.md)。

## 下一步

- [第 7 章：模型训练](07-model-training.md)
- [第 10 章：边缘智能模块](10-edge-intelligence.md)