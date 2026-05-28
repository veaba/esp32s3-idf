# QRCode ESP32-S3 项目上下文

## 项目目标
在 ESP32-S3 SPI LCD 上生成并显示 WiFi QRCode，手机可扫描连接 WiFi。

## 参考实现
- Rust/WASM/JS: `F:\Github\veaba\qrcodes` (本地)
- JS 共享核心: `F:\Github\veaba\qrcodes\packages\qrcode-js-shared\src\index.ts`
- 参考 QRCode 类: `QRCodeCore`

## 当前状态
**数据编码和 RS 编码已验证正确**，但生成的 QRCode 仍无法被手机扫描。

## 已修复的 Bug

### 1. RS 编码 (qrcode.c:66-74)
**Bug**: `buffer[i]` 在 j=0 循环中被修改后，j>0 用了修改后的值。
**Fix**: 保存 feedback = buffer[i]，然后用 `gen[ec_count - 1 - j]` 反向访问生成多项式。
```c
// 修复后
for (i = 0; i < data_len; i++) {
    uint8_t feedback = buffer[i];
    if (feedback == 0) continue;
    for (j = 0; j < ec_count; j++) {
      buffer[i + j + 1] ^= gf_mul(feedback, gen[ec_count - 1 - j]);
    }
}
```

### 2. Format Info EC 级别映射 (qrcode.c:295-296)
**Bug**: 枚举值 `M=1` 直接乘 8 作为表索引，但表顺序是 M,L,H,Q。
**Fix**: 添加映射表 `ec_fmt_map[] = {1, 0, 3, 2}`。

### 3. Quiet Zone (qrcode.c:447-450)
**Bug**: 缺少 QRCode 规范要求的 4 模块白色边框。
**Fix**: 在 `qrcode_show_on_lcd` 中添加 quiet=4 的白色边框。

### 4. Format Info 位序和位置 (qrcode.c:299-320) - 最新修复
**Bug**: 
- 用了 MSB-first `(fmt_bits >> (14 - i))` 而非 LSB-first `(fmt_bits >> i)`
- 位置公式错误（行/列计算不对）

**Fix**: 完全重写，匹配参考实现：
```c
for (i = 0; i < 15; i++) {
    int bit = (fmt_bits >> i) & 1;  // LSB first
    // Vertical strip (col 8)
    if (i < 6) { r = i; c = 8; }
    else if (i < 8) { r = i + 1; c = 8; }  // skip row 6
    else { r = size - 15 + i; c = 8; }
    matrix[r][c] = bit;
    // Horizontal strip (row 8)
    if (i < 8) { r = 8; c = size - i - 1; }
    else if (i < 9) { r = 8; c = 7; }
    else { r = 8; c = 14 - i; }  // skip col 6
    matrix[r][c] = bit;
}
```

### 5. Format Info 位置未标记为 Function Module - 最新修复
**Bug**: Format info 位置（row 8, col 8 附近）未标记为 function module，导致 data placement 占用了 format info 的位置，数据位移。
**Fix**: 在 `place_function_patterns` 中标记 format info 位置：
```c
for (i = 0; i <= 8; i++) {
    qr_is_function[i][8] = 1;
    qr_is_function[8][i] = 1;
}
for (i = 0; i < 7; i++) {
    qr_is_function[size - 1 - i][8] = 1;
    qr_is_function[8][size - 1 - i] = 1;
}
```

## 验证结果（最新 flash）

### 数据字节（完全匹配参考）
```
C:    40 B6 86 56 C6 C6 F2 07 76 F7 26 C6 40 EC 11 EC
JS:   40 B6 86 56 C6 C6 F2 07 76 F7 26 C6 40 EC 11 EC ✓
```

### EC 字节（完全匹配参考）
```
C:    39 3A DC 20 D5 08 C5 FA 3F C1
JS:   39 3A DC 20 D5 08 C5 FA 3F C1 ✓
```

### 矩阵（不同，因为 mask 不同）
- JS: mask=0, C: mask=3
- Function patterns (finder, timing) ✓
- Format info 和 data area 不同

## 待验证
最新修复（format info 位序 + function module 标记）尚未刷写测试。

## 文件清单

### C 实现
- `components/BSP/QRCODE/qrcode.h` — 头文件，QRCode 结构体，API 声明
- `components/BSP/QRCODE/qrcode.c` — 核心实现（GF 表、RS 编码、mask、LCD 渲染）
- `components/BSP/QRCODE/CMakeLists.txt` — 构建配置
- `components/BSP/CMakeLists.txt` — BSP 组件列表
- `main/main.c` — 入口，显示 "hello world" QRCode

### 参考实现
- `F:\Github\veaba\qrcodes\packages\qrcode-js-shared\src\index.ts` — QRCodeCore 类
- `F:\Github\veaba\qrcodes\packages\qrcode-js-shared\test-hello.mjs` — 测试脚本

## QRCode 规范要点（Version 1, 21x21）

### 数据容量
- Version 1, EC Level M: 16 data codewords, 10 EC codewords, 1 block
- 总共 26 codewords = 208 bits

### Format Info
- 15 bits: EC level (2 bits) + mask pattern (3 bits) + BCH error correction (10 bits)
- 放置位置: col 8 (rows 0-5, 7-8, 14-20) 和 row 8 (cols 20-13, 7, 5-0)
- XOR mask: 0x5412

### Data Placement
- 从右下角开始，之字形（zigzag）向左上角
- 跳过 col 6（timing pattern）
- 跳过 function modules

### Mask Patterns
- 8 种 mask 模式，选择 penalty 最小的
- Mask 只应用于 data modules（非 function modules）

## 构建/刷写
```bash
# 构建
idf.py build

# 刷写+监视
idf.py -p COM4 flash monitor

# 串口输出 QR 标签可查看矩阵
```

## 下一步
1. 刷写最新代码（format info 修复 + function module 标记）
2. 检查串口输出的矩阵是否与参考匹配（同 mask 下）
3. 如果仍然失败，检查 data placement 的 zigzag 顺序
