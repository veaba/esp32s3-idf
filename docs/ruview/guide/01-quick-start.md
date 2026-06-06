# 第 1 章：快速入门

本章帮助你在最短时间内运行 RuView 并验证系统工作正常。

---

## 30 秒演示（Docker）

无需硬件、无需编译，一条命令启动：

```bash
docker pull ruvnet/wifi-densepose:latest
docker run -p 3000:3000 -p 3001:3001 ruvnet/wifi-densepose:latest
```

打开浏览器访问 http://localhost:3000 ，你将看到：

- 3D 人体骨架（17 个 COCO 关键点）
- 信号振幅热力图
- 相位图
- 生命体征面板（呼吸率 + 心率）

## 验证系统运行

打开第二个终端，测试 API：

```bash
# 健康检查
curl http://localhost:3000/health
# 返回: {"status":"ok","source":"simulated","clients":0}

# 最新感知帧
curl http://localhost:3000/api/v1/sensing/latest

# 生命体征
curl http://localhost:3000/api/v1/vital-signs

# 当前姿态（17 COCO 关键点）
curl http://localhost:3000/api/v1/pose/current

# 服务器构建信息
curl http://localhost:3000/api/v1/info
```

所有端点返回 JSON。模拟模式下数据由确定性参考信号生成。

## 管道验证（无硬件）

验证信号处理管道是真实且确定性的：

```bash
# 方式一：shell 包装脚本
./verify

# 方式二：直接运行 Python 脚本
python archive/v1/data/proof/verify.py
```

验证流程包含三个阶段：

1. **环境检查** — 确认 Python、numpy、scipy 和证明文件存在
2. **管道回放** — 将发布的参考信号送入完整信号处理链（噪声过滤、Hamming 窗、振幅归一化、FFT 多普勒提取、功率谱密度），计算输出 SHA-256 哈希
3. **代码完整性扫描** — 扫描生产代码中是否存在 `np.random.rand` / `np.random.randn` 调用

退出码：

| 退出码 | 含义 |
|--------|------|
| 0 | 通过 — 管道哈希与预期哈希匹配 |
| 1 | 失败 — 哈希不匹配或出错 |
| 2 | 跳过 — 无预期哈希文件可供比较 |

详细输出：

```bash
./verify --verbose         # 详细特征统计和多普勒频谱
./verify --verbose --audit # 完整验证 + 代码审计
```

如需重新生成预期哈希：

```bash
python archive/v1/data/proof/verify.py --generate-hash
```

## 源码构建快速启动

```bash
git clone https://github.com/ruvnet/RuView.git
cd RuView/v2

# 构建
cargo build --release

# 运行测试（1400+ 测试）
cargo test --workspace --no-default-features

# 启动感知服务器（模拟模式）
./target/release/sensing-server --source simulate --http-port 3000 --ws-port 3001
```

## 下一步

- [第 2 章：安装与构建](02-installation.md) — 详细安装选项
- [第 3 章：数据源配置](03-data-sources.md) — 连接真实硬件
- [第 4 章：REST API](04-rest-api.md) — 完整 API 参考