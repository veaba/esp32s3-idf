# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32-S3 firmware project (ATK-DNESP32S3 V1.2 dev board) that drives an SPI LCD display with Chinese character rendering. Uses ESP-IDF v5.5.4 build system.

This project uses a **精简版中文字库** (3502 characters) instead of the full GB2312 set (20902 chars), saving ~700KB of flash. The `show_chinese_string()` API uses callback-based pixel and ASCII drawing for flexible rendering.

## Build & Flash Commands

```bash
# Build the project
idf.py build

# Flash to device (default COM4 via UART)
idf.py flash monitor

# Flash only
idf.py -p COM4 -b 115200 flash

# Clean build artifacts
idf.py fullclean

# Monitor serial output
idf.py -p COM4 monitor
```

## Chinese Font Regeneration

The 16x16 Chinese character font data is in `components/BSP/CHINESE16_3K/chinese16_3k.h` (auto-generated, 3502 characters). To regenerate:

```bash
cd ../code_diff
uv run scripts/generate_chinese_3k.py
```

## Hardware Configuration

| Peripheral | Pin/Address |
|---|---|
| SPI2 MOSI | GPIO11 |
| SPI2 CLK | GPIO12 |
| SPI2 MISO | GPIO13 |
| LCD WR | GPIO40 |
| LCD CS | GPIO21 |
| I2C0 SDA | GPIO41 |
| I2C0 SCL | GPIO42 |
| XL9555 I2C addr | 0x20 |
| LED | GPIO1 |
| SPI LCD clock | 60 MHz |

## Project Structure

```
15_chinese_3k/
├── main/
│   └── main.c                  # Entry point: NVS init, LED, I2C, SPI, LCD, XL9555 init, then display loop
├── components/
│   └── BSP/
│       ├── SPI/                # SPI2 bus init + polled transmit (cmd/data/byte)
│       ├── LCD/                # LCD driver: init, pixel/char/string drawing, text box (auto-wrap)
│       │   ├── lcd.c           # Core LCD driver + TextBox auto-wrapping
│       │   ├── lcd.h           # LCD API, color defs, scan dirs, TextBox struct
│       │   └── lcdfont.h       # ASCII font tables (12/16/24/32 px)
│       ├── CHINESE16_3K/       # 16x16 Chinese character font (精简版, 3502 chars, auto-generated)
│       │   └── chinese16_3k.h  # Unicode index table + 32-byte bitmap per character
│       ├── IIC/                # I2C master driver (generic init + transfer for any I2C device)
│       ├── XL9555/             # 16-bit I/O expander driver (pin read/write, key scan)
│       └── LED/                # On-board LED control (GPIO1)
├── partitions-16MiB.csv        # 16MB flash: factory(2MB), vfs/fat(10MB), storage/spiffs(4MB)
├── CMakeLists.txt              # Root: sets EXTRA_COMPONENT_DIRS, auto-names project from dir name
├── all-chinese.png             # Screenshot: Chinese text displayed in full font mode
├── no-all-chinese.png          # Screenshot: Chinese text without full font
└── sdkconfig                   # ESP-IDF config (generated, do not edit manually)
```

## Key Architecture

### Initialization Order (main.c)

1. `nvs_flash_init()` — NVS storage
2. `led_init()` — onboard LED (GPIO1)
3. `iic_init(I2C_NUM_0)` — I2C0 master (GPIO41/42, 400kHz)
4. `spi2_init()` — SPI2 bus (GPIO11/12/13, DMA auto)
5. `xl9555_init()` — I/O expander over I2C0, configures pins, enables beeper/speaker
6. `lcd_init()` — SPI LCD: adds SPI device (60MHz), sends init cmds, sets direction, enables backlight

### LCD Display

- SPI interface via `spi_device_polling_transmit` (polled mode)
- 2.4" LCD (ILI9341-like, 240x320) or 3" LCD, selectable via `SPI_LCD_TYPE` define
- 16-bit RGB565 color format
- Buffer: 15360 bytes for bulk fill, total frame 153600 bytes (320*240*2)
- ASCII: 4 font sizes (12/16/24/32) from `lcdfont.h`
- Chinese: 16x16 only, from `chinese16_3k.h` (3502 common characters)
- `TextBox` struct provides auto-wrapping text regions

### Chinese Character Rendering (精简版)

- Font data in `components/BSP/CHINESE16_3K/chinese16_3k.h` contains a Unicode index table (`chinese_unicode_table[]`) and a flat bitmap array (`chinese_bitmap[]`, 32 bytes per char = 16x16 pixels)
- Only 3502 most common Chinese characters (vs 20902 in full GB2312 set), saving ~700KB of flash
- The function `show_chinese_string()` is declared in `chinese16_3k.h` — takes x, y, string, foreground color, background color, pixel draw callback, and ASCII draw callback
- Font generation tool lives in `../code_diff/scripts/generate_chinese_3k.py`

### I/O Expander (XL9555)

- I2C device at address 0x20, drives: LCD backlight/复位/电源, buzzer, speaker, camera, touch, keys (KEY0-3)
- Key scanning with debounce: `xl9555_key_scan(mode)` — returns KEY0_PRES through KEY3_PRES

### Partition Table

- 16MB flash: factory app (2MB), FAT for VFS (10MB), SPIFFS for storage (4MB)

### LCD Pin Connections (via XL9555)

| XL9555 Pin | Function |
|---|---|
| SLCD_PWR_IO (0x0800) | LCD power |
| SLCD_RST_IO (0x0400) | LCD reset |
| LCD_BL_IO (0x0100) | LCD backlight |
| BEEP_IO (0x0008) | Buzzer |
| SPK_EN_IO (0x0004) | Speaker enable |

## Flash Usage Comparison

| Font | Characters | .rodata | Image Size |
|---|---|---|---|
| No Chinese font | 0 | baseline | ~269 KB |
| Full GB2312 (14_chinese_all) | 20902 | +752 KB | ~1.0 MB |
| Lite 3K (this project) | 3502 | ~112 KB | ~381 KB |

## VSCode Configuration

- ESP-IDF extension, ESP32-S3 target, clangd for code intelligence
- Compile flags: `-ffast-math -O3 -Wno-error=format -Wno-format`
- clangd removes `-f*` and `-m*` flags for analysis
