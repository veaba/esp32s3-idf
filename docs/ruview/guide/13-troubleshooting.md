# 第 13 章：故障排除

---

## ESP32 节点不出现在 /api/v1/nodes

**症状**：ESP32-S3 节点关联 WiFi，LED 闪烁，但没有 CSI 帧到达服务器。

**原因**：USB 刷写后节点进入跛行状态 — WiFi 关联但 UDP CSI 发送器静默失败。

**修复**：重新上电节点（拔掉 USB，等 2 秒，重新插入）。如不行，通过串口 DTR 重置：`python -m serial.tools.miniterm --dtr 0 COMx 115200`，然后 Ctrl+C。

**预防**：固件 0.8.0+ 包含看门狗，30 秒无 CSI 帧时自动触发软件重置。

## 人数计数卡在 1

**症状**：`estimated_persons` 始终返回 1，无论房间内有多少人。

**原因**（ADR-044）：8 个汇聚错误：
1. `score_to_person_count` 上限为 3
2. `fuse_multi_node_features` 使用 `.max()` 而非求和 — N 个相同读数坍缩为 1
3. 四个 `.max(1)` 钳制强制最小计数为 1
4. `field_model.estimate_occupancy` 上限 `.min(3)`
5. 归一化饱和（用硬编码阈值除以而非自适应 p95）
6. 无场模型自动校准
7. 生命体征路径钳制不对称
8. 层析产生一个块（CC=1）

**修复**：已在 Waves 1-3 中逐步修复。当前状态：5 人场景报告 6-8（含宠物），过度计数因 dedup 因子是估算值。

## 心率/呼吸率抖动

**症状**：HR 和 BR 每帧之间剧烈跳动。

**原因**（ADR-045）：11 个 ESP32 节点各自独立计算生命体征，服务器使用 last-write-wins。

**修复**（`46fbc061`）：最佳节点选择。每个节点的生命体征通过中值滤波 + EMA 独立平滑。选择 `breathing_confidence + heartbeat_confidence` 最高的节点作为权威源。结果：BR CV 23.3% → 12.6%，HR CV 12.9% → 11.6%。

**已知限制**：`wifi-densepose-vitals` crate 有更优的 4 阶段管道（带通 → 希尔伯特包络 → 自相关 → 峰值检测），但尚未接入感知服务器。

## 信号质量始终 50%

**症状**：仪表板信号质量仪表始终卡在 ~50%。

**原因**：信号质量是硬编码占位值，未从实际 CSI 数据派生。

**修复**：ADR-044 Wave 2 替换为 RollingP95 自适应归一化。UI 诚实化通过（`b2070ab4`）添加了 beta 标签和实际每节点数据。

## 仪表板每 2-4 秒冻结

**症状**：空间视图和仪表板冻结然后重连，造成可见卡顿。

**原因**：WebSocket 广播频道 `recv()` 在客户端落后时返回 `Err(Lagged)`，服务器将其视为致命错误并断开连接。

**修复**（`581daf4f`）：
- 服务器：`Lagged` 错误 → `continue`（跳过未读帧而非断开）
- 服务器：30 秒 ping/pong 保活

## OTA 更新在 59% 崩溃

**症状**：OTA 固件更新推进到 ~59% 时节点以 `StoreProhibited` 在 Core 1 上崩溃。

**原因**：NimBLE BLE 广告/扫描运行在 Core 1。OTA 期间 HTTP 客户端也运行在 Core 1。BLE 和 OTA 竞争栈空间。

**修复**：
1. 在调用 `esp_https_ota_begin()` 前停止 NimBLE 广告和扫描
2. 将 httpd 栈从 4KB 增加到 8KB
3. OTA 完成或失败后恢复 BLE

**注意**：运行旧固件的节点无法通过 OTA 接收此修复。必须先 USB 刷写固件 0.8.0+。

## 右侧 USB-C 端口不工作

**症状**：插入右侧 USB-C 端口时主机不显示串口设备。

**修复**：使用左侧 USB-C 端口。左侧是 USB-to-UART 桥（CP2102/CH340），用于刷写和串口监视器。右侧是原生 USB（USB-JTAG），RuView 固件不使用。

## `cargo test` 构建失败

**症状**：`cargo test --workspace` 因 BLAS/eigenvalue 错误而失败。

**修复**：必须使用 `--no-default-features` 标志：

```bash
cargo test --workspace --no-default-features
```

`eigenvalue` 特性需要 BLAS（ndarray-linalg + OpenBLAS），可能不可用。

## 下一步

- [第 14 章：常见问题解答](14-faq.md)