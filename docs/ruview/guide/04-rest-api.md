# 第 4 章：REST API 参考

基础 URL：`http://localhost:3000`（Docker）或 `http://localhost:8080`（二进制默认）。

---

## 核心 API

| 方法 | 端点 | 说明 |
|------|------|------|
| `GET` | `/health` | 服务器健康检查 |
| `GET` | `/api/v1/sensing/latest` | 最新 CSI 感知帧（振幅、相位、运动） |
| `GET` | `/api/v1/vital-signs` | 呼吸率 + 心率 + 置信度 |
| `GET` | `/api/v1/pose/current` | 17 COCO 关键点（x, y, z, confidence） |
| `GET` | `/api/v1/info` | 服务器版本、构建信息、运行时间 |
| `GET` | `/api/v1/bssid` | 多 BSSID WiFi 注册表 |

## 模型管理

| 方法 | 端点 | 说明 |
|------|------|------|
| `GET` | `/api/v1/models` | 列出可用 RVF 模型文件 |
| `GET` | `/api/v1/models/active` | 当前加载的模型 |
| `POST` | `/api/v1/models/load` | 按 ID 加载模型 |
| `POST` | `/api/v1/models/unload` | 卸载当前模型 |
| `DELETE` | `/api/v1/models/:id` | 删除模型文件 |
| `GET` | `/api/v1/model/layers` | 渐进式模型加载状态 |
| `GET` | `/api/v1/model/sona/profiles` | SONA 适配配置文件 |
| `POST` | `/api/v1/model/sona/activate` | 激活指定房间的 SONA 配置 |
| `GET` | `/api/v1/models/lora/profiles` | 列出 LoRA 适配器配置 |
| `POST` | `/api/v1/models/lora/activate` | 激活 LoRA 配置 |

## 录制与训练

| 方法 | 端点 | 说明 |
|------|------|------|
| `GET` | `/api/v1/recording/list` | 列出 CSI 录制会话 |
| `POST` | `/api/v1/recording/start` | 开始录制 CSI 帧到 JSONL |
| `POST` | `/api/v1/recording/stop` | 停止当前录制 |
| `DELETE` | `/api/v1/recording/:id` | 删除录制文件 |
| `GET` | `/api/v1/train/status` | 训练运行状态 |
| `POST` | `/api/v1/train/start` | 开始训练运行 |
| `POST` | `/api/v1/train/stop` | 停止当前训练 |

## 自适应分类器

| 方法 | 端点 | 说明 |
|------|------|------|
| `POST` | `/api/v1/adaptive/train` | 从录制数据训练自适应分类器 |
| `GET` | `/api/v1/adaptive/status` | 模型状态和准确率 |
| `POST` | `/api/v1/adaptive/unload` | 卸载模型，恢复阈值分类 |

## 示例

### 获取生命体征

```bash
curl -s http://localhost:3000/api/v1/vital-signs | python -m json.tool
```

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

### 获取姿态

```bash
curl -s http://localhost:3000/api/v1/pose/current | python -m json.tool
```

```json
{
    "persons": [
        {
            "id": 0,
            "keypoints": [
                {"name": "nose", "x": 0.52, "y": 0.31, "z": 0.0, "confidence": 0.91},
                {"name": "left_eye", "x": 0.54, "y": 0.29, "z": 0.0, "confidence": 0.88}
            ]
        }
    ],
    "frame_id": 1024,
    "timestamp_ms": 1709312400000
}
```

### 录制训练数据

```bash
# 开始录制
curl -X POST http://localhost:3000/api/v1/recording/start \
  -H "Content-Type: application/json" -d '{"id":"train_empty_room"}'

# 等待 30 秒...

# 停止录制
curl -X POST http://localhost:3000/api/v1/recording/stop
```

## 下一步

- [第 5 章：WebSocket 实时流](05-websocket.md)
- [第 8 章：自适应分类器](08-adaptive-classifier.md)