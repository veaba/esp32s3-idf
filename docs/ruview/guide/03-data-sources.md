# 第 3 章：数据源配置

RuView 支持多种数据源，通过 `--source` 标志控制。

---

## 模拟模式（无硬件）

默认模式（Docker 中）。生成合成 CSI 数据，运行完整管道。

```bash
# Docker
docker run -p 3000:3000 ruvnet/wifi-densepose:latest

# 源码
./target/release/sensing-server --source simulate --http-port 3000 --ws-port 3001
```

## Windows WiFi（仅 RSSI）

使用 `netsh wlan` 捕获附近 AP 的 RSSI。无需特殊硬件。支持存在检测、运动分类和粗略呼吸率估计。不支持姿态估计（需要 CSI）。

```bash
# 源码（仅 Windows）
./target/release/sensing-server --source wifi --http-port 3000 --ws-port 3001 --tick-ms 500

# Docker
docker run --network host ruvnet/wifi-densepose:latest --source wifi --tick-ms 500
```

> 社区验证：已在 Windows 10 (10.0.26200) + Intel Wi-Fi 6 AX201 160MHz 上测试通过。参见 [Issue #36](https://github.com/ruvnet/RuView/issues/36)。

## macOS WiFi（仅 RSSI）

使用 CoreWLAN（通过 Swift 辅助二进制）。macOS Sonoma 14.4+ 会编辑真实 BSSID；适配器生成确定性合成 MAC。

```bash
# 编译 Swift 辅助（一次性）
swiftc -O archive/v1/src/sensing/mac_wifi.swift -o mac_wifi

# 运行
./target/release/sensing-server --source macos --http-port 3000 --ws-port 3001 --tick-ms 500
```

## Linux WiFi（仅 RSSI）

使用 `iw dev <iface> scan` 捕获 RSSI。主动扫描需要 `CAP_NET_ADMIN`（root）。

```bash
# 需要 root 进行主动扫描
sudo ./target/release/sensing-server --source linux --http-port 3000 --ws-port 3001 --tick-ms 500
```

## ESP32-S3（完整 CSI）

真实信道状态信息，20 Hz，56-192 子载波。姿态估计、生命体征、穿墙感知的必选项。

```bash
# 源码
./target/release/sensing-server --source esp32 --udp-port 5005 --http-port 3000 --ws-port 3001

# Docker
docker run -p 3000:3000 -p 3001:3001 -p 5005:5005/udp \
  -e CSI_SOURCE=esp32 ruvnet/wifi-densepose:latest
```

ESP32 节点通过 UDP 5005 端口传输二进制 CSI 帧。参见 [第 6 章：硬件设置](06-hardware-setup.md) 刷写说明。

## ESP32 多态 Mesh（高级）

3-6 个 ESP32-S3 节点组成多态 mesh 配置，提升穿墙追踪精度。

```bash
./target/release/sensing-server --source esp32 --udp-port 5005 --http-port 3000 --ws-port 3001
```

Mesh 使用 TDM（时分复用）协议让节点轮流发射，避免自干扰：

| 特性 | 说明 |
|------|------|
| TDM 协调 | 节点循环 TX/RX 时隙（可配置保护间隔） |
| 信道跳频 | 自动 2.4/5 GHz 频段循环实现多频融合 |
| QUIC 传输 | 聚合节点上 TLS 1.3 加密流（ADR-032a） |
| 手动加密回退 | 受限 ESP32-S3 节点上 HMAC-SHA256 信标认证 |
| 注意力加权融合 | 跨视点注意力 + 几何多样性偏置 |

详见 [ADR-029](../adr/ADR-029-ruvsense-multistatic-sensing-mode.md) 和 [ADR-032](../adr/ADR-032-multistatic-mesh-security-hardening.md)。

## 下一步

- [第 4 章：REST API](04-rest-api.md) — 完整 API 参考
- [第 6 章：硬件设置](06-hardware-setup.md) — ESP32 刷写与配置