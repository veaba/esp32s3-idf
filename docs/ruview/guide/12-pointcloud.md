# 第 12 章：3D 点云

RuView 可以通过融合摄像头深度估计与 WiFi CSI 空间感知生成实时 3D 点云。

---

## 设置

```bash
# 构建点云二进制
cd v2
cargo build --release -p wifi-densepose-pointcloud

# 启动服务器（自动检测摄像头 + CSI）。默认仅本地回环。
./target/release/ruview-pointcloud serve --bind 127.0.0.1:9880
```

打开 http://localhost:9880 查看交互式 Three.js 3D 查看器。

> **安全提示**：服务器通过 HTTP 暴露实时摄像头、骨架、生命体征和占用数据。`--bind` 默认为 `127.0.0.1:9880`（仅本地回环）。如需远程访问，请使用反向代理。

## 传感器

| 传感器 | 自动检测 | 数据 |
|--------|----------|------|
| 摄像头 (`/dev/video0`) | 是（Linux UVC） | RGB 帧 → MiDaS 深度 → 3D 点 |
| ESP32 CSI (UDP:3333) | 是（如已预置） | ADR-018 二进制 → 占用 + 姿态 + 生命体征 |
| MiDaS 深度服务器 (端口 9885) | 可选 | GPU 加速神经深度估计 |

## 命令

| 命令 | 说明 |
|------|------|
| `ruview-pointcloud serve --bind 127.0.0.1:9880` | 启动 HTTP 服务器 + Three.js 查看器 |
| `ruview-pointcloud demo` | 生成合成点云（无需硬件） |
| `ruview-pointcloud capture --output room.ply` | 捕获单帧到 PLY 文件 |
| `ruview-pointcloud cameras` | 列出可用摄像头 |
| `ruview-pointcloud train --data-dir ./data` | 深度校准 + 占用训练 |
| `ruview-pointcloud csi-test --count 100` | 发送测试 CSI 帧 |
| `ruview-pointcloud fingerprint <name> [--seconds 5]` | 录制命名 CSI 房间指纹 |

## API 端点

| 端点 | 方法 | 返回 |
|------|------|------|
| `/health` | GET | `{"status": "ok"}` |
| `/api/status` | GET | 摄像头、CSI、管道状态、生命体征、运动 |
| `/api/cloud` | GET | 点云（最多 1000 点）+ 管道数据 |
| `/api/splats` | GET | Three.js 渲染用高斯飞溅 |

## 管道组件

1. **ADR-018 解析器** — 从 UDP 解码 ESP32 CSI 二进制帧，提取 I/Q 子载波振幅和相位
2. **姿态（占位）** — 从 CSI 振幅能量生成 17 COCO 关键点布局（非训练模型，占位）
3. **生命体征** — 从 CSI 相位分析提取呼吸率（稳定子载波峰值计数）
4. **运动检测** — CSI 振幅方差（20 帧窗口），触发自适应捕获
5. **RF 层析** — 从每节点 RSSI 反投影到 8×8×4 占用网格
6. **摄像头深度** — MiDaS 单目深度（GPU）+ 亮度+边缘回退
7. **传感器融合** — 摄像头深度 + CSI 占用的体素网格合并
8. **Brain Bridge** — 每 60 秒将空间观测存储到 ruOS brain

## 训练

```bash
ruview-pointcloud train --data-dir ~/.local/share/ruview/training --brain http://127.0.0.1:9876
```

捕获帧、运行深度校准（比例/偏移/伽马网格搜索）、训练占用阈值、导出 DPO 偏好对、提交结果。

## 输出格式

- **PLY** — 标准 3D 点云（ASCII，含 RGB 颜色）
- **Gaussian Splats** — Three.js 渲染用 JSON 格式
- **Brain Memories** — 存储为 `spatial-observation`、`spatial-motion`、`spatial-vitals`

## 深度房间扫描

```bash
# 先停止实时服务器（释放摄像头）
# 然后捕获 20 帧并用 MiDaS 处理
ruview-pointcloud capture --frames 20 --output room_model.ply
```

结果：40,000+ 体素（5cm 分辨率），12,000+ 高斯飞溅。

## ESP32 预置

```bash
python3 firmware/esp32-csi-node/provision.py \
    --port /dev/ttyACM0 \
    --ssid "YourWiFi" --password "YourPassword" \
    --target-ip 192.168.1.123 --target-port 3333 \
    --node-id 1
```

## 下一步

- [第 4 章：REST API](04-rest-api.md)
- [第 6 章：硬件设置](06-hardware-setup.md)