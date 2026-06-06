# 第 9 章：生命体征检测

系统从 CSI 信号波动中提取呼吸率和心率。

---

## 检测能力

| 体征 | 频率范围 | 范围 | 方法 |
|------|----------|------|------|
| 呼吸率 | 0.1-0.5 Hz | 6-30 BPM | 带通滤波 + FFT 峰值 |
| 心率 | 0.8-2.0 Hz | 40-120 BPM | 带通滤波 + FFT 峰值 |

## 精度要求

- 需要 CSI 硬件（ESP32-S3 或研究 NIC）才能获得准确读数
- 受试者需在接入点 ~3-5 米范围内（多态 mesh 可达 ~8m）
- 受试者需相对静止（大幅运动会掩盖生命体征振荡）
- 模拟模式产生合成生命体征数据用于测试

## 信号平滑

生命体征估计经过三阶段平滑管道（ADR-048）：

1. **异常值拒绝** — 每帧 ±8 BPM HR, ±2 BPM BR
2. **21 帧修剪均值** — 移除极端值后取中间值
3. **EMA** — α=0.02 指数移动平均

稳定读数可持续 5-10+ 秒而非每帧跳动。

## API 使用

```bash
# 获取生命体征
curl http://localhost:3000/api/v1/vital-signs
```

响应示例：

```json
{
    "breathing_bpm": 16.2,
    "heart_bpm": 72.1,
    "breathing_confidence": 0.87,
    "heart_confidence": 0.63,
    "motion_level": 0.12,
    "timestamp_ms": 1709312400000
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `breathing_bpm` | 浮点 | 呼吸率（每分钟呼吸次数） |
| `heart_bpm` | 浮点 | 心率（每分钟心跳次数） |
| `breathing_confidence` | 浮点 | 呼吸率置信度 0.0-1.0 |
| `heart_confidence` | 浮点 | 心率置信度 0.0-1.0 |
| `motion_level` | 浮点 | 运动水平 0.0-1.0 |
| `timestamp_ms` | 整数 | 时间戳（毫秒） |

## ESP32 边缘生命体征

启用边缘 Tier 2 后，ESP32 每秒发送 32 字节生命体征包（可配置），包含：

- 存在状态
- 运动评分
- 呼吸 BPM
- 心率 BPM
- 置信度值
- 跌倒标志
- 占用人数估计

```bash
# 启用边缘 Tier 2 生命体征
python firmware/esp32-csi-node/provision.py --port COM7 \
  --ssid "YourWiFi" --password "secret" --target-ip 192.168.1.20 \
  --edge-tier 2
```

## 最佳实践

- 确保受试者静止（运动中的生命体征不可靠）
- 距离 AP 3-5 米效果最佳
- 多节点 mesh 提高精度（取置信度最高的节点数据）
- 使用自适应分类器进行环境校准
- 心率检测置信度通常低于呼吸率（0.6 vs 0.87）

## 下一步

- [第 6 章：硬件设置](06-hardware-setup.md)
- [第 8 章：自适应分类器](08-adaptive-classifier.md)