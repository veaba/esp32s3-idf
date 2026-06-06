# 第 8 章：自适应分类器

自适应分类器（ADR-048）从标记录制数据学习你的环境的特定 WiFi 信号模式，替代静态阈值分类。

---

## 信号平滑管道

所有 CSI 派生指标在到达 UI 前经过三阶段管道：

| 阶段 | 功能 | 关键参数 |
|------|------|----------|
| 自适应基线 | 学习静室噪声底，减去漂移 | α=0.003，50 帧预热 |
| EMA + 中值滤波 | 平滑运动评分和生命体征 | 运动 α=0.15；生命体征：21 帧修剪均值，α=0.02 |
| 滞后消抖 | 防止状态快速跳变 | 4 帧（~0.4s）状态转换确认 |

生命体征额外稳定参数：

| 参数 | 值 | 效果 |
|------|-----|------|
| HR 死区 | ±2 BPM | 防止微漂 |
| BR 死区 | ±0.5 BPM | 防止微漂 |
| HR 最大跳变 | 8 BPM/帧 | 拒绝噪声尖峰 |
| BR 最大跳变 | 2 BPM/帧 | 拒绝噪声尖峰 |

## 录制训练数据

录制标记 CSI 会话，每种活动录制约 30 秒：

```bash
# 1. 空房间（离开房间 30 秒）
curl -X POST http://localhost:3000/api/v1/recording/start \
  -H "Content-Type: application/json" -d '{"id":"train_empty_room"}'
# ... 等待 30 秒 ...
curl -X POST http://localhost:3000/api/v1/recording/stop

# 2. 静坐（靠近 ESP32 静坐 30 秒）
curl -X POST http://localhost:3000/api/v1/recording/start \
  -H "Content-Type: application/json" -d '{"id":"train_sitting_still"}'
# ... 等待 30 秒 ...
curl -X POST http://localhost:3000/api/v1/recording/stop

# 3. 行走（在房间内走动 30 秒）
curl -X POST http://localhost:3000/api/v1/recording/start \
  -H "Content-Type: application/json" -d '{"id":"train_walking"}'
# ... 等待 30 秒 ...
curl -X POST http://localhost:3000/api/v1/recording/stop

# 4. 剧烈运动（开合跳、挥手 30 秒）
curl -X POST http://localhost:3000/api/v1/recording/start \
  -H "Content-Type: application/json" -d '{"id":"train_active"}'
# ... 等待 30 秒 ...
curl -X POST http://localhost:3000/api/v1/recording/stop
```

### 文件名规则

录制保存为 `data/recordings/` 中的 JSONL 文件。文件名必须以 `train_` 开头并包含类关键词：

| 文件名模式 | 类别 |
|------------|------|
| `*empty*` 或 `*absent*` | 缺席 |
| `*still*` 或 `*sitting*` | 在场静坐 |
| `*walking*` 或 `*moving*` | 在场移动 |
| `*active*` 或 `*exercise*` | 活动 |

## 训练模型

```bash
curl -X POST http://localhost:3000/api/v1/adaptive/train
```

服务器使用 mini-batch SGD（200 轮）在 15 个特征上训练多类逻辑回归。训练在典型录制集上不到 1 秒完成。模型保存到 `data/adaptive_model.json` 并在服务器重启时自动加载。

### 查看模型状态

```bash
curl http://localhost:3000/api/v1/adaptive/status
```

### 卸载模型（恢复阈值分类）

```bash
curl -X POST http://localhost:3000/api/v1/adaptive/unload
```

## 使用训练好的模型

训练完成后模型自动运行：

1. 每个 CSI 帧使用学习到的权重分类（而非静态阈值）
2. 模型置信度与平滑阈值置信度混合（70/30）
3. 模型在服务器重启后持久化（从 `data/adaptive_model.json` 加载）

### 提高准确率的技巧

- 录制时每种活动有明显区分（空房间要真的离开）
- 每种活动录制 30-60 秒（数据越多越好）
- 移动 ESP32 或重新布置房间后重新录制和训练
- 模型是环境特定的 — 物理设置变化时需重新训练

## API 参考

| 方法 | 端点 | 说明 |
|------|------|------|
| `POST` | `/api/v1/adaptive/train` | 从 `train_*` 录制训练 |
| `GET` | `/api/v1/adaptive/status` | 模型状态、准确率、类统计 |
| `POST` | `/api/v1/adaptive/unload` | 卸载模型，恢复阈值 |
| `POST` | `/api/v1/recording/start` | 开始录制 CSI 帧 |
| `POST` | `/api/v1/recording/stop` | 停止录制 |
| `GET` | `/api/v1/recording/list` | 列出录制 |

## 下一步

- [第 4 章：REST API](04-rest-api.md)
- [第 9 章：生命体征检测](09-vital-signs.md)