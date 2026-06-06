# 第 14 章：常见问题解答

---

## 通用

**Q：RuView 是什么？**

A：RuView（WiFi-DensePose）是一个 WiFi 感知平台，将无线电信号转化为空间智能。它使用 ESP32 传感器捕获的 CSI 来检测人员、测量呼吸和心率、追踪运动、监控房间 — 穿墙、暗光、无需摄像头或可穿戴设备。

**Q：需要硬件才能使用吗？**

A：不需要。Docker 镜像包含模拟数据模式，无需硬件。仅需 CSI 硬件（ESP32-S3）进行姿态估计、生命体征和穿墙感知。

**Q：哪些 ESP32 板受支持？**

A：ESP32-S3（8MB 或 4MB flash）。ESP32-C3 和原始 ESP32 不受支持（单核，无法运行 CSI DSP 管道）。

**Q：单个 ESP32 能做什么？**

A：单个 ESP32-S3 可以检测存在、粗略运动和估计呼吸率。多人计数、姿态估计和心跳需要 3+ 节点的多态 mesh。

**Q：信号处理管道是否确定性？**

A：是的。`verify` 脚本将参考信号通过完整管道，产生可复现的 SHA-256 哈希。生产代码中不允许使用 `np.random.rand`/`np.random.randn`。

## 构建与运行

**Q：`cargo test` 失败，怎么办？**

A：使用 `--no-default-features`：

```bash
cd v2
cargo test --workspace --no-default-features
```

**Q：WASM 边缘 crate 构建失败？**

A：`wifi-densepose-wasm-edge` 从工作区排除。单独构建：

```bash
cargo build -p wifi-densepose-wasm-edge --target wasm32-unknown-unknown --release
```

**Q：Python 证明哈希不匹配？**

A：如果 numpy 或 scipy 版本改变，哈希可能改变。重新生成：

```bash
python archive/v1/data/proof/verify.py --generate-hash
python archive/v1/data/proof/verify.py
```

**Q：Linux 构建需要哪些系统依赖？**

A：对于桌面/Tauri crate：

```bash
sudo apt install -y build-essential pkg-config libglib2.0-dev libgtk-3-dev \
  libsoup-3.0-dev libjavascriptcoregtk-4.1-dev libwebkit2gtk-4.1-dev
```

## 硬件

**Q：如何刷写 ESP32-S3 固件？**

A：参见 [第 6 章：硬件设置](06-hardware-setup.md)。核心命令：

```bash
python -m esptool --chip esp32s3 --port COM7 --baud 460800 \
  write-flash --flash-mode dio --flash-size 8MB --flash-freq 80m \
  0x0 bootloader.bin 0x8000 partition-table.bin \
  0xf000 ota_data_initial.bin 0x20000 esp32-csi-node.bin
```

**Q：如何预置 WiFi 凭据？**

A：

```bash
python firmware/esp32-csi-node/provision.py --port COM7 \
  --ssid "YourWiFi" --password "secret" --target-ip 192.168.1.20
```

**Q：右侧 USB-C 端口不工作？**

A：使用左侧端口。右侧是 USB-JTAG，左侧是 CP2102/CH340 UART 桥。

**Q：如何监控 ESP32 串口输出？**

A：`python -m serial.tools.miniterm COM7 115200`

## API

**Q：默认端口是什么？**

A：Docker：HTTP 3000 + WebSocket 3001。二进制：HTTP 8080 + WebSocket 8765。所有可通过命令行标志配置。

**Q：WebSocket 端点是什么？**

A：`ws://localhost:3000/ws/sensing`（与 HTTP 同端口）或 `ws://localhost:3001/ws/sensing`（专用端口）。

**Q：如何录制 CSI 数据？**

A：

```bash
curl -X POST http://localhost:3000/api/v1/recording/start \
  -H "Content-Type: application/json" -d '{"id":"my_recording"}'
# ... 等待 ...
curl -X POST http://localhost:3000/api/v1/recording/stop
```

## 训练

**Q：需要 GPU 训练吗？**

A：不需要。训练管道完全用 Rust 实现，零外部 ML 依赖。lite 规模 189K 参数在笔记本电脑上 ~19 分钟完成。

**Q：支持哪些数据集？**

A：MM-Fi（NeurIPS 2023，40 受试者，4 房间）和 Wi-Pose（Entropy 2023，12 受试者，1 房间）。

**Q：如何在新房间使用模型？**

A：使用 MERIDIAN 10 秒自动校准。无需标签、无需重训练、无需用户干预。

**Q：可以不用摄像头训练吗？**

A：可以。无摄像头训练使用 10 种传感器信号生成弱标签。参见 [第 7 章](07-model-training.md) 中的无摄像头训练部分。

## 生命体征

**Q：心率检测准确吗？**

A：取决于硬件。多节点 ESP32 mesh 在静坐受试者 3-5 米范围内可达到 ~0.6 置信度。不准确时系统返回低置信度。

**Q：可以在有 WiFi 的任何房间使用吗？**

A：可以，室内 WiFi 路由器产生的多径效应实际上是 CSI 感知的必要条件。空旷空间效果较差。

## 更多帮助

- [GitHub Issues](https://github.com/ruvnet/RuView/issues)
- [GitHub Discussions](https://github.com/ruvnet/RuView/discussions)
- [完整用户指南](../user-guide.md)