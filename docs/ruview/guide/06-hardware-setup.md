# 第 6 章：ESP32 硬件设置

3-6 个 ESP32-S3 节点组成的 mesh 提供完整 CSI（20 Hz，56-192 子载波），总成本约 $54。

---

## 物料清单

| 物品 | 数量 | 单价 | 小计 |
|------|------|------|------|
| ESP32-S3-DevKitC-1 | 3 | $10 | $30 |
| USB-A 转 USB-C 数据线 | 3 | $3 | $9 |
| 多口 USB 电源适配器 | 1 | $15 | $15 |
| WiFi 路由器（任意） | 1 | $0（已有） | $0 |
| 聚合器（笔记本或树莓派） | 1 | $0（已有） | $0 |

## 固件刷写

预编译固件在 [Releases](https://github.com/ruvnet/RuView/releases) 下载：

| 版本 | 说明 | 标签 |
|------|------|------|
| v0.5.0 | 稳定版（推荐）— mmWave 传感器融合、48 字节融合生命体征、所有修复 | `v0.5.0-esp32` |
| v0.4.3.1 | 跌倒检测修复、4MB flash 支持、看门狗修复 | `v0.4.3.1-esp32` |

> **重要**：始终使用 v0.4.3.1 或更新版本。早期版本存在跌倒误报和 CSI 构建配置问题。

### 8MB Flash 刷写（大多数开发板）

```bash
python -m esptool --chip esp32s3 --port COM7 --baud 460800 \
  write-flash --flash-mode dio --flash-size 8MB --flash-freq 80m \
  0x0 bootloader.bin 0x8000 partition-table.bin \
  0xf000 ota_data_initial.bin 0x20000 esp32-csi-node.bin
```

### 4MB Flash 刷写（ESP32-S3 SuperMini 等）

```bash
python -m esptool --chip esp32s3 --port COM7 --baud 460800 \
  write-flash --flash-mode dio --flash-size 4MB --flash-freq 80m \
  0x0 bootloader.bin 0x8000 partition-table-4mb.bin \
  0xF000 ota_data_initial.bin 0x20000 esp32-csi-node-4mb.bin
```

## 预置 WiFi

```bash
python firmware/esp32-csi-node/provision.py --port COM7 \
  --ssid "YourWiFi" --password "YourPassword" --target-ip 192.168.1.20
```

将 `192.168.1.20` 替换为运行感知服务器的机器 IP。

## Mesh 密钥预置（安全模式）

多态 mesh 部署需要共享密钥进行 HMAC-SHA256 信标认证：

```bash
python firmware/esp32-csi-node/provision.py --port COM7 \
  --ssid "YourWiFi" --password "YourPassword" --target-ip 192.168.1.20 \
  --mesh-key "$(openssl rand -hex 32)"
```

同一 mesh 中所有节点必须使用相同的 256 位密钥。密钥存储在 ESP32 NVS flash 中，固件擦除时会清零。

## TDM 时隙分配

多态 mesh 中每个节点需要唯一的 TDM 时隙 ID（从 0 开始）：

```bash
# 节点 0（时隙 0）— 第一个发射器
python firmware/esp32-csi-node/provision.py --port COM7 --tdm-slot 0 --tdm-total 3

# 节点 1（时隙 1）
python firmware/esp32-csi-node/provision.py --port COM8 --tdm-slot 1 --tdm-total 3

# 节点 2（时隙 2）
python firmware/esp32-csi-node/provision.py --port COM9 --tdm-slot 2 --tdm-total 3
```

## 边缘智能（固件 v0.3.0+）

边缘处理在 ESP32 上本地运行，无需主机 PC：

| 等级 | 功能 | 额外 RAM |
|------|------|----------|
| 0 | 禁用（默认）— 仅将原始 CSI 流传到聚合器 | 0 KB |
| 1 | 相位展开、统计、Top-K 子载波选择、增量压缩 | ~30 KB |
| 2 | Tier 1 全部 + 存在检测、呼吸/心率、运动评分、跌倒检测 | ~33 KB |

通过 NVS 启用（无需重新刷写）：

```bash
python firmware/esp32-csi-node/provision.py --port COM7 \
  --ssid "YourWiFi" --password "YourPassword" --target-ip 192.168.1.20 \
  --edge-tier 2
```

### NVS 边缘处理参数

| NVS 键 | 默认值 | 说明 |
|---------|--------|------|
| `edge_tier` | 0 | 处理等级（0=关闭, 1=统计, 2=生命体征） |
| `pres_thresh` | 50 | 存在检测灵敏度（越低越灵敏） |
| `fall_thresh` | 15000 | 跌倒检测阈值（毫单位，15000 = 15.0 rad/s²） |
| `vital_win` | 300 | 相位历史窗口大小（帧数） |
| `vital_int` | 1000 | 生命体征包发送间隔（毫秒） |
| `subk_count` | 32 | 保留的最佳子载波数量（总共 56 个中） |

## 启动聚合器

```bash
# 源码
./target/release/sensing-server --source esp32 --udp-port 5005 --http-port 3000 --ws-port 3001

# Docker
docker run -p 3000:3000 -p 3001:3001 -p 5005:5005/udp \
  -e CSI_SOURCE=esp32 ruvnet/wifi-densepose:latest
```

## ESP32 Mesh 能力矩阵

| 能力 | 1 节点 | 3 节点 | 6 节点 |
|------|--------|--------|--------|
| 存在检测 | 好 | 优秀 | 优秀 |
| 粗略运动 | 好 | 优秀 | 优秀 |
| 房间级定位 | 无 | 好 | 优秀 |
| 呼吸检测 | 一般 | 好 | 好 |
| 心跳检测 | 差 | 差 | 一般 |
| 多人计数 | 无 | 一般 | 好 |
| 姿态估计 | 无 | 差 | 一般 |

## 下一步

- [第 9 章：生命体征检测](09-vital-signs.md)
- [第 10 章：边缘智能模块](10-edge-intelligence.md)