# ESP32 Code Comparator

基于 AST 的 ESP-IDF 项目代码深度对比工具，专为 ESP32 开发调试设计。

## 特性

- 🔍 **AST 级别对比**：不仅是文本，深入语法结构
- 🌐 **函数调用图**：可视化调用关系差异
- 🤖 **智能建议**：基于差异自动生成修复建议
- 🔄 **代码重构**：生成标准化的版本C代码
- 📊 **多种报告格式**：JSON、HTML、终端可视化

## 快速开始

### 安装 uv

```bash
# macOS/Linux
curl -LsSf https://astral.sh/uv/install.sh | sh

# Windows
powershell -c "irm https://astral.sh/uv/install.ps1 | iex"

# 或使用 pip
pip install uv


```

## 使用

```shell
# 对比两个项目
uv run esp32-diff /path/to/reference /path/to/your_project

# uv run esp32-diff ../10_spi_lcd ../10_spilcd

# 生成 JSON 报告
uv run esp32-diff ./ref_project ./my_project -f json -o ./report

# 生成 HTML 可视化报告
uv run esp32-diff ./ref_project ./my_project -f html

# 生成重构代码
uv run esp32-diff ./ref_project ./my_project -r

# 查看帮助
uv run esp32-diff --help
```

### 单个文件对比

```shell
uv run vscode-diff  ../10_spi_lcd/main/main.c ../10_spilcd/main/main.c
```

```shell
uv run vscode-diff  ../10_spilcd/components/BSP/LCD/lcd.c ../10_spi_lcd/components/BSP/LCD/lcd.c
uv run vscode-diff  ../10_spilcd/components/BSP/LCD/lcd.h ../10_spi_lcd/components/BSP/LCD/lcd.h
uv run vscode-diff  ../10_spilcd/components/BSP/SPI/spi.h ../10_spi_lcd/components/BSP/SPI/spi.h
uv run vscode-diff  ../10_spilcd/components/BSP/SPI/spi.c ../10_spi_lcd/components/BSP/SPI/spi.c


```
