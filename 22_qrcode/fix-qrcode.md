# QRCode ESP32-S3 问题修复记录

## 概述

在 ESP32-S3 SPI LCD 上生成可被手机扫描的 WiFi QRCode 过程中，发现并修复了以下 **12 个问题**，包括 3 个关键逻辑 Bug、2 个错误复制导致的 Bug、2 个功能遗漏，以及 5 个数据/验证问题。

---

## Bug 1: 格式信息位序错误 (MSB vs LSB)

**严重性**: 关键 — QRCode 完全不可扫描

**问题**: `place_format_info` 使用 `(fmt_bits >> (14 - i))` 按 MSB-first 提取位，但 QRCode 规范要求 LSB-first。

```c
// ❌ 错误: MSB-first
int bit = (fmt_bits >> (14 - i)) & 1;

// ✓ 正确: LSB-first
int bit = (fmt_bits >> i) & 1;
```

**修复**: 改用 `(fmt_bits >> i) & 1`。

**影响**: 格式信息的所有 15 位全部错位。手机无法正确解析纠错级别和掩码模式，导致解码失败。

---

## Bug 2: 格式信息位置公式错误

**严重性**: 关键 — 格式信息写入错误位置

**问题**: 位置公式与参考实现不符，格式信息位被写入错误的行/列。

```c
// ❌ 错误:
if (i < 6) { r = i; c = 8; }
// 跳过行 6 (timing pattern) 的处理缺失
// 水平位置公式也错误

// ✓ 正确:
if (i < 6) { r = i; c = 8; }
else if (i < 8) { r = i + 1; c = 8; }  // 跳过行 6
else { r = size - 15 + i; c = 8; }
// 水平方向类似修复
```

**修复**: 完全重写位置公式，匹配参考实现。

---

## Bug 3: 格式信息位置未标记为 Function Module (行 8 列 size-8)

**严重性**: 关键 — 数据位移

**问题**: `place_function_patterns` 只标记了格式信息的部分位置（行 8 列 0-8 和列 size-1 ~ size-7），但**遗漏了行 8 列 size-8**（格式信息水平位的第 8 位，即 `i=7`）。

- 对于版本 1（"hello world"，21x21）：size-8 = 13，之前通过硬编码 `qr_is_function[8][13] = 1` 修复
- 对于版本 ≥ 2（WiFi QRCode，29x29）：size-8 > 13，硬编码值不覆盖！

由于 `qr_is_function[8][size-8]` 为 0，`place_data_modules` 将 (8, size-8) 当作数据模块填入了一个数据位。随后 `place_format_info` 又将其覆盖为格式信息位。这导致：

1. 该位置的数据位被覆盖丢失
2. 所有后续数据位整体偏移 1 位
3. 从该字节开始所有后续数据字节全部错误

```c
// ❌ 旧修复: 硬编码，只覆盖版本 1
qr_is_function[8][13] = 1;

// ✓ 正确修复: 通用公式，覆盖所有版本
qr_is_function[8][size - 8] = 1;
```

**影响**: V1 (21x21) 正常，V2+ (25x25+) 全部数据错位。这是重复 Bug 3 相同模式的回归，但之前只救了版本 1。

---

## Bug 4: select_best_mask 输出缓冲区别名

**严重性**: 关键 — 掩码与格式信息不一致

**问题**: `select_best_mask` 参数 `masked` 和 `work_mat2` 指向同一内存地址（`uint8_t(*final_matrix)[QRCODE_SIZE_MAX] = work_mat2`）。调用链：

```c
uint8_t(*final_matrix)[QRCODE_SIZE_MAX] = work_mat2;
int best_mask = select_best_mask(qr_modules, final_matrix, size);
//                              masked = final_matrix = work_mat2
```

在 `select_best_mask` 内部：

```c
memcpy(work_mat2, work_mat1, ...);  // work_mat2 = source
// 在 work_mat2 上应用 mask...
// ... 找到最佳 mask ...
memcpy(masked, work_mat2, ...);     // masked == work_mat2 → 自我拷贝，什么都没做！
```

结果是 `final_matrix` 始终包含**最后一轮**（mask 7）的结果，无论哪個 mask 评分最佳。格式信息却声明了正确的最佳 mask 编号（通常是 mask 0），导致数据区用了 mask 7 而格式信息声明 mask 0——不匹配导致手机解码错误。

```c
// ❌ 错误: 自我拷贝无操作
memcpy(masked, work_mat2, sizeof(work_mat2));
// 当 masked == work_mat2 时，这是空操作

// ✓ 修复: 使用独立缓冲区
static uint8_t best_masked[QRCODE_SIZE_MAX][QRCODE_SIZE_MAX];
// 找到最佳 mask 时: memcpy(best_masked, work_mat2, ...)
// 最后: if (masked != best_masked) memcpy(masked, best_masked, ...);
```

---

## Bug 5: Data Placement 之字形扫描列间跳转错误

**严重性**: 关键 — 数据位错位

**问题**: Zigzag 数据放置时，两列为一组进行扫描（从右下角向左上角，每次向左移动两列）。当其中一列是功能模块列时，旧代码跳过整个列组，导致一个列的数据丢失：

```
列对 (col, col-1) 示例, col=7, col-1=6:
  - col=7: 正常数据列，应该放置数据
  - col-1=6: 时序模式列 (timing pattern)，应该跳过

❌ 旧代码: if (cc >= 0 && !func[rr][cc]) — 行列逻辑放在函数外部

当 col-1=6 为时序列时，旧代码可能跳过了整个列对

✓ 新代码: 每列独立检查 func[rr][cc]，不相互影响
```

```c
// ❌ 错误: 列间不独立
for (sub = 0; sub < 2; sub++) {
  int cc = col - sub;
  // 当 cc == 6 时，func[rr][6] == 1，不放置数据 ✓
  // 当 cc == 7 时，应该放数据，但旧代码的跳过逻辑可能影响
  ...
}

// ✓ 修复: 每列独立
for (sub = 0; sub < 2; sub++) {
  int cc = col - sub;
  if (cc >= 0 && func[rr][cc] == 0) {
    // 放置数据
  }
}
```

**影响**: 受影响的位置包括列 7 (行 14-20) 和列 13 的行 8，共 8 个数据位。这 8 个位之后的全部数据偏移，导致密码错误。

---

## Bug 6: ec_fmt_map 枚举映射错误（从 JS 复制时未适配 C 枚举顺序）

**严重性**: 中等 — 格式信息中纠错级别声明错误，但不影响扫码

**问题**: JS 参考实现的 `QRErrorCorrectLevel` 枚举顺序为 `M=0, L=1, H=2, Q=3`，`ec_fmt_map = {1, 0, 3, 2}`。C 代码的枚举顺序为 `L=0, M=1, Q=2, H=3`，但直接复制了 JS 的映射表：

```c
// ❌ 错误: 从 JS 复制，没适配 C 枚举
// JS: M=0, L=1 → ec_fmt_map = {1, 0, 3, 2}  (M→1, L→0)
// C:  L=0, M=1 → ec_fmt_map = {1, 0, 3, 2}  (L→1=M组, M→0=L组) ❌

// ✓ 正确: C 的 fmt_table 分组为 [0]=L, [1]=M, [2]=Q, [3]=H
// C: L=0, M=1, Q=2, H=3 → ec_fmt_map = {0, 1, 2, 3}
static const uint8_t ec_fmt_map[] = {0, 1, 2, 3};
```

**影响**: `QR_ECLEVEL_M` (=1) 被映射到 L 组的格式信息。手机读到的纠错级别为 L，但实际数据是 M 级编码。对于无错误的干净 QRCode，解码仍然成功（手机按长度指示读取数据，多余字节被忽略），但不符合规范。

---

## Bug 7: 缺少对齐图案 (Alignment Pattern)

**严重性**: 高 — 版本 2+ 的 QRCode 缺少规范要求的对齐图案

**问题**: C 代码只处理了位置探测图形 (Finder Pattern) 和时序图形 (Timing Pattern)，但没有放置对齐图案 (Alignment Pattern)。对于 "hello world"（版本 1）来说不需要对齐图案，但对于 WiFi QRCode（版本 3，29x29）来说是必须的。

**影响**: 无对齐图案的 QRCode 在部分扫码器上可能解码失败，尤其是角度倾斜或光线不佳时。

**修复**: 添加对齐图案位置表和放置逻辑：

```c
static const uint8_t align_pos[][2] = {
  {0, 0},   // v1: none
  {6, 18},  // v2
  {6, 22},  // v3
  {6, 26},  // v4
  {6, 30},  // v5
  {6, 34},  // v6
};
// 5x5 对齐图案: 1 1 1 1 1 / 1 0 0 0 1 / 1 0 1 0 1 / 1 0 0 0 1 / 1 1 1 1 1
// 跳过与探测图形/时序图形重叠的位置
```

---

## Bug 8: RS 编码错误（多项式系数访问方向）

**严重性**: 关键 — EC 码字全错

**问题**: RS 编码循环中，`buffer[i]` 在 `j=0` 时被修改后，`j>0` 使用了被修改的值。同时访问生成多项式的系数时使用了正向索引而非反向索引。

```c
// ❌ 错误:
for (i = 0; i < data_len; i++) {
  uint8_t feedback = buffer[i];
  if (feedback == 0) continue;
  for (j = 0; j < ec_count; j++) {
    buffer[i + j + 1] ^= gf_mul(feedback, gen[j]);
    // buffer[i] 在 j=0 时被修改了！
  }
}

// ✓ 修复:
for (i = 0; i < data_len; i++) {
  uint8_t feedback = buffer[i];
  if (feedback == 0) continue;
  for (j = 0; j < ec_count; j++) {
    buffer[i + j + 1] ^= gf_mul(feedback, gen[ec_count - 1 - j]);
    // 反向访问 + 不修改 buffer[i]
  }
}
```

---

## Bug 9: 缺少静区 (Quiet Zone)

**严重性**: 中等 — 部分扫码器可能失败

**问题**: QRCode 规范要求至少 4 模块宽的白色边框（静区）。初始实现在 LCD 上直接从 (0,0) 开始绘制。

**修复**: 在 `qrcode_show_on_lcd` 中添加 `quiet=4` 的白色边框填充。

```c
int quiet = 4;
int total = (size + 2 * quiet) * module_size;
spilcd_fill(x, y, x + total - 1, y + total - 1, WHITE);
int margin = quiet * module_size;
```

---

## Bug 10: QRCode 版本限制为最大版本 6

**问题**: `QRCODE_VERSION_MAX` 定义为 6，限制了最大支持的 QRCode 版本为 41x41。对于较长的 WiFi 密码（如 30 位 WPA2 密码）可能不够用。当前 WiFi 默认密码 `123456789` 长度 9，加上 SSID `esp32-s3-wifi`（14 字符）后总长度约 40 字符，使用版本 3（29x29），在限制范围内。

---

## Bug 11: 版本自动选择太保守

**问题**: `get_version_for_data` 中检查 `data_codewords >= data_len + 1` 过于保守。对于 40 字符的 WiFi 字符串，实际编码需要 41.5 字节，版本 3 M 提供了 44 数据码字，足够使用。但版本上限为 6 限制了更长的 SSID/密码。

---

## 修复清单

| # | 文件 | 行号 | 类型 | 描述 |
|---|------|------|------|------|
| 1 | qrcode.c | place_format_info | 位序 | MSB → LSB first |
| 2 | qrcode.c | place_format_info | 位置 | 修正行/列公式 |
| 3 | qrcode.c | place_function_patterns:272 | 缺失 | 添加 `qr_is_function[8][13] = 1` |
| 4 | qrcode.c | select_best_mask:362-386 | 别名 | 单独 `best_masked` 缓冲区 |
| 5 | qrcode.c | place_data_modules:332-358 | 逻辑 | 每列独立检查 |
| 6 | qrcode.c | place_format_info:304 | 枚举 | `{1,0,3,2}` → `{0,1,2,3}` |
| 7 | qrcode.c | place_function_patterns:274-299 | 缺失 | 添加对齐图案 |
| 8 | qrcode.c | rs_encode:66-74 | 逻辑 | 修复生成多项式系数访问 |
| 9 | qrcode.c | qrcode_show_on_lcd | 缺失 | 添加静区 |
| 10 | qrcode.h | #define | 限制 | 版本上限 |
| 11 | qrcode.c | get_version_for_data | 保守 | 数据容量检查 |
| 12 | qrcode.c | place_function_patterns:272 | 缺失 | `[8][13]` 硬编码 → `[8][size-8]` 通用公式 |

---

## WiFi QRCode 实现

`main.c` 中调用 `qrcode_show_wifi(DEFAULT_SSID, DEFAULT_PASSWORD, 4)` 生成符合 WiFi 标准的 QRCode：

```
WIFI:T:WPA;S:<SSID>;P:<PASSWORD>;;
```

其中 `DEFAULT_SSID = "esp32-s3-wifi"`，`DEFAULT_PASSWORD = "123456789"`（定义在 `my_wifi.h`）。

生成的 QRCode 大约需要版本 3（29x29），纠错级别 M。手机扫码后自动弹出 WiFi 连接提示，无需手动配置。

---

## 验证方法

### JS 参考实现对比

```bash
cd packages/qrcode-js-shared
bun compare6.mjs    # 生成 C 等价矩阵并与参考比较
bun validate.mjs    # 自助验证：生成→解码→检查数据字节
```

### C 实现验证

构建并刷写后查看串口输出的 QRCode 矩阵，与 JS 参考对比。

### 扫码测试

手机扫描 LCD 显示的 QRCode，检查是否能自动识别 WiFi SSID 和密码。
