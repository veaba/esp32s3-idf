# 第 5 章：WebSocket 实时流

实时感知数据通过 WebSocket 推送。

---

## 连接地址

| URL | 说明 |
|-----|------|
| `ws://localhost:3000/ws/sensing` | 与 HTTP 同端口（推荐） |
| `ws://localhost:3001/ws/sensing` | 专用 WebSocket 端口（向后兼容） |

Web UI 使用 HTTP 端口，只需暴露一个端口。

## Python 示例

```python
import asyncio
import websockets
import json

async def stream():
    uri = "ws://localhost:3001/ws/sensing"
    async with websockets.connect(uri) as ws:
        async for message in ws:
            data = json.loads(message)
            persons = data.get("persons", [])
            vitals = data.get("vital_signs", {})
            print(f"人员数: {len(persons)}, "
                  f"呼吸率: {vitals.get('breathing_bpm', 'N/A')} BPM")

asyncio.run(stream())
```

## JavaScript 示例

```javascript
const ws = new WebSocket("ws://localhost:3001/ws/sensing");

ws.onmessage = (event) => {
    const data = JSON.parse(event.data);
    console.log("人员数:", data.persons?.length ?? 0);
    console.log("呼吸率:", data.vital_signs?.breathing_bpm, "BPM");
};

ws.onerror = (err) => console.error("WebSocket 错误:", err);
```

## curl 单帧（需 wscat）

```bash
# 安装 wscat
npm install -g wscat

# 连接并接收一帧
wscat -c ws://localhost:3001/ws/sensing
```

## 消息格式

每帧 JSON 消息包含：

| 字段 | 类型 | 说明 |
|------|------|------|
| `persons` | 数组 | 检测到的人员列表（含关键点） |
| `vital_signs` | 对象 | 呼吸率、心率、置信度 |
| `frame_id` | 整数 | 帧序号 |
| `timestamp_ms` | 整数 | 时间戳（毫秒） |
| `motion_level` | 浮点 | 运动水平 0.0-1.0 |
| `estimated_persons` | 整数 | 估计人数 |
| `source` | 字符串 | 数据源（`simulated`/`esp32`/`wifi`） |

## 下一步

- [第 4 章：REST API](04-rest-api.md)
- [第 9 章：生命体征检测](09-vital-signs.md)