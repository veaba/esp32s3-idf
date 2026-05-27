#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
从系统字体提取中文字库，生成16x16点阵字库 (ESP-IDF版本)
使用方法: python3 gen_chinese_font.py
"""

from PIL import Image, ImageDraw, ImageFont
import os

# ==================== 配置参数 ====================
UNICODE_TABLE_FILE = "F:/hardware-design/esp32s3-idf/code_diff/scripts/chinese_unicode.txt"
OUTPUT_H_FILE = "chinese16.h"
FONT_SIZE = 16
FONT_PATH = "F:/hardware-design/esp32s3-idf/code_diff/scripts/simhei.ttf"

# ==================== 1. 解析Unicode表 ====================


def parse_unicode_table(filename):
    """
    解析unicode.txt文件
    返回: [(汉字, Unicode码点), ...]
    """
    chinese_chars = []

    with open(filename, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    for i in range(0, len(lines), 2):
        if i < len(lines):
            chars_line = lines[i].strip()
            chars = chars_line.split()

            if i+1 < len(lines):
                codes_line = lines[i+1].strip()
                codes = codes_line.split()

                for j, ch in enumerate(chars):
                    if j < len(codes):
                        unicode_val = int(codes[j], 16)
                        if 0x4E00 <= unicode_val <= 0x9FFF:
                            chinese_chars.append((ch, unicode_val))

    print(f"✓ 解析到 {len(chinese_chars)} 个汉字")
    return chinese_chars

# ==================== 2. 生成点阵数据 ====================


def char_to_bitmap(char, font, size=16):
    """
    将单个字符转换为16x16点阵数据
    返回: 32字节的点阵数据 (16行 * 2字节/行)
    """
    img = Image.new('1', (size, size), 0)
    draw = ImageDraw.Draw(img)

    # 获取文字实际大小并居中
    bbox = draw.textbbox((0, 0), char, font=font)
    text_width = bbox[2] - bbox[0]
    text_height = bbox[3] - bbox[1]
    x = (size - text_width) // 2 - bbox[0]
    y = (size - text_height) // 2 - bbox[1]
    draw.text((x, y), char, font=font, fill=1)

    # 转换为点阵数据
    bitmap = []
    for y in range(size):
        byte1 = 0
        byte2 = 0
        for x in range(size):
            if img.getpixel((x, y)):
                if x < 8:
                    byte1 |= (1 << (7 - x))
                else:
                    byte2 |= (1 << (15 - x))
        bitmap.append(byte1)
        bitmap.append(byte2)

    return bitmap

# ==================== 3. 生成头文件 (ESP-IDF版本) ====================


def generate_header_file(chinese_chars, output_file, font, font_size=16):
    """
    生成chinese16.h头文件 (ESP-IDF版本，无Arduino依赖)
    """
    with open(output_file, 'w', encoding='utf-8') as f:
        # 文件头
        f.write("// 自动生成的中文字库文件 (ESP-IDF版本)\n")
        f.write("// 字体大小: {}x{} 点阵\n".format(font_size, font_size))
        f.write("// 汉字数量: {}\n".format(len(chinese_chars)))
        f.write("// 生成时间: 请勿手动修改\n\n")

        f.write("#ifndef _CHINESE16_H_\n")
        f.write("#define _CHINESE16_H_\n\n")

        f.write("#include <stdint.h>\n")
        f.write("#include <string.h>\n\n")

        # ========== 1. Unicode索引表 ==========
        f.write("// Unicode索引表\n")
        f.write("static const uint32_t chinese_unicode_table[] = {\n    ")

        for i, (ch, code) in enumerate(chinese_chars):
            f.write(f"0x{code:04X}")
            if i < len(chinese_chars) - 1:
                f.write(", ")
            if (i + 1) % 10 == 0:
                f.write("\n    ")
        f.write("\n};\n\n")

        # ========== 2. 点阵数据表 ==========
        f.write("// 16x16点阵字库数据 (每字32字节)\n")
        f.write("static const uint8_t chinese_font_data[][32] = {\n")

        for i, (ch, code) in enumerate(chinese_chars):
            if (i + 1) % 500 == 0:
                print(f"  生成点阵: {i+1}/{len(chinese_chars)}")

            bitmap = char_to_bitmap(ch, font, font_size)

            f.write(f"    // 汉字: {ch} (U+{code:04X})\n")
            f.write("    { ")
            for j, byte in enumerate(bitmap):
                f.write(f"0x{byte:02X}")
                if j < 31:
                    f.write(", ")
                if (j + 1) % 16 == 0 and j < 31:
                    f.write("\n      ")
            f.write(" },\n\n")

        f.write("};\n\n")

        # ========== 3. 辅助函数 (ESP-IDF版本) ==========
        f.write("// UTF-8转Unicode\n")
        f.write("static inline uint32_t utf8_to_unicode(const char* str) {\n")
        f.write("    if ((str[0] & 0xF0) == 0xE0) {\n")
        f.write(
            "        return ((str[0] & 0x0F) << 12) | ((str[1] & 0x3F) << 6) | (str[2] & 0x3F);\n")
        f.write("    } else if ((str[0] & 0xE0) == 0xC0) {\n")
        f.write("        return ((str[0] & 0x1F) << 6) | (str[1] & 0x3F);\n")
        f.write("    } else {\n")
        f.write("        return str[0];\n")
        f.write("    }\n")
        f.write("}\n\n")

        f.write("// 二分查找汉字索引\n")
        f.write("static inline int get_chinese_index(uint32_t unicode) {\n")
        f.write("    int left = 0;\n")
        f.write(
            "    int right = sizeof(chinese_unicode_table) / sizeof(uint32_t) - 1;\n")
        f.write("    while (left <= right) {\n")
        f.write("        int mid = (left + right) / 2;\n")
        f.write("        if (chinese_unicode_table[mid] == unicode) {\n")
        f.write("            return mid;\n")
        f.write("        } else if (chinese_unicode_table[mid] < unicode) {\n")
        f.write("            left = mid + 1;\n")
        f.write("        } else {\n")
        f.write("            right = mid - 1;\n")
        f.write("        }\n")
        f.write("    }\n")
        f.write("    return -1;\n")
        f.write("}\n\n")

        f.write("// 绘制16x16汉字点阵 (使用回调函数)\n")
        f.write(
            "typedef void (*draw_pixel_cb)(uint16_t x, uint16_t y, uint16_t color);\n\n")

        f.write(
            "static inline void draw_chinese_bitmap(uint16_t x, uint16_t y,\n")
        f.write("                                       const uint8_t* bitmap,\n")
        f.write(
            "                                       uint16_t color, uint16_t bg_color,\n")
        f.write(
            "                                       draw_pixel_cb draw_pixel) {\n")
        f.write("    for (int row = 0; row < 16; row++) {\n")
        f.write("        for (int col = 0; col < 16; col++) {\n")
        f.write("            uint8_t byte_idx = row * 2 + (col / 8);\n")
        f.write("            uint8_t bit_mask = 0x80 >> (col % 8);\n")
        f.write("            if (bitmap[byte_idx] & bit_mask) {\n")
        f.write("                draw_pixel(x + col, y + row, color);\n")
        f.write("            } else if (bg_color != 0xFFFF) {\n")
        f.write("                draw_pixel(x + col, y + row, bg_color);\n")
        f.write("            }\n")
        f.write("        }\n")
        f.write("    }\n")
        f.write("}\n\n")

        f.write("// 显示单个汉字\n")
        f.write("static inline void show_chinese_char(uint16_t x, uint16_t y,\n")
        f.write("                                    const char* ch,\n")
        f.write(
            "                                    uint16_t color, uint16_t bg_color,\n")
        f.write(
            "                                    draw_pixel_cb draw_pixel) {\n")
        f.write("    uint32_t unicode = utf8_to_unicode(ch);\n")
        f.write("    int idx = get_chinese_index(unicode);\n")
        f.write("    if (idx >= 0) {\n")
        f.write(
            "        draw_chinese_bitmap(x, y, chinese_font_data[idx], color, bg_color, draw_pixel);\n")
        f.write("    }\n")
        f.write("}\n\n")

        f.write("// 显示中文字符串\n")
        f.write(
            "static inline void show_chinese_string(uint16_t x, uint16_t y,\n")
        f.write("                                     const char* str,\n")
        f.write(
            "                                     uint16_t color, uint16_t bg_color,\n")
        f.write("                                     draw_pixel_cb draw_pixel,\n")
        f.write(
            "                                     void (*draw_ascii)(uint16_t, uint16_t, char, uint16_t, uint16_t)) {\n")
        f.write("    uint16_t cursor_x = x;\n")
        f.write("    uint16_t cursor_y = y;\n")
        f.write("    while (*str) {\n")
        f.write("        if ((*str & 0x80) == 0) {\n")
        f.write("            if (draw_ascii) {\n")
        f.write(
            "                draw_ascii(cursor_x, cursor_y, *str, color, bg_color);\n")
        f.write("            }\n")
        f.write("            cursor_x += 8;\n")
        f.write("            str++;\n")
        f.write("        } else {\n")
        f.write("            char ch[4] = {0};\n")
        f.write("            int len = 0;\n")
        f.write("            if ((*str & 0xF0) == 0xE0) len = 3;\n")
        f.write("            else if ((*str & 0xE0) == 0xC0) len = 2;\n")
        f.write("            if (len > 0) {\n")
        f.write("                memcpy(ch, str, len);\n")
        f.write(
            "                show_chinese_char(cursor_x, cursor_y, ch, color, bg_color, draw_pixel);\n")
        f.write("                cursor_x += 16;\n")
        f.write("                str += len;\n")
        f.write("            } else {\n")
        f.write("                str++;\n")
        f.write("            }\n")
        f.write("        }\n")
        f.write("    }\n")
        f.write("}\n\n")

        f.write("#endif // _CHINESE16_H_\n")

    print(f"✓ 字库生成成功: {output_file}")
    print(
        f"  总大小: {len(chinese_chars) * 32} 字节 (~{len(chinese_chars) * 32 / 1024:.1f} KB)")

# ==================== 查找可用字体 ====================


def find_available_font():
    """自动查找系统中可用的中文字体"""
    possible_fonts = [
        "C:/Windows/Fonts/simhei.ttf",
        "C:/Windows/Fonts/msyh.ttc",
        "C:/Windows/Fonts/simsun.ttc",
        "/System/Library/Fonts/PingFang.ttc",
        "/System/Library/Fonts/STHeiti Light.ttc",
        "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc",
        "/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc",
    ]

    for font_path in possible_fonts:
        if os.path.exists(font_path):
            return font_path
    return None

# ==================== 主程序 ====================


def main():
    print("=" * 50)
    print("中文字库生成器 v2.0 (ESP-IDF版本)")
    print("=" * 50)

    font_path = FONT_PATH

    # 1. 检查字体文件
    if not os.path.exists(font_path):
        print(f"⚠ 字体文件不存在: {font_path}")
        print("正在自动查找可用字体...")

        found_font = find_available_font()
        if found_font:
            font_path = found_font
            print(f"✓ 找到可用字体: {font_path}")
        else:
            print("\n❌ 未找到任何中文字体文件！")
            print("请手动设置 FONT_PATH 变量")
            return

    # 2. 加载字体
    try:
        font = ImageFont.truetype(font_path, FONT_SIZE)
        print(f"✓ 加载字体成功: {font_path}")
    except Exception as e:
        print(f"❌ 字体加载失败: {e}")
        return

    # 3. 解析Unicode表
    if not os.path.exists(UNICODE_TABLE_FILE):
        print(f"❌ 找不到文件: {UNICODE_TABLE_FILE}")
        return

    chinese_chars = parse_unicode_table(UNICODE_TABLE_FILE)
    if len(chinese_chars) == 0:
        print("❌ 未解析到任何汉字")
        return

    # 4. 生成字库文件
    generate_header_file(chinese_chars, OUTPUT_H_FILE, font, FONT_SIZE)

    print("\n" + "=" * 50)
    print("✓ 完成！")
    print(f"  生成的文件: {OUTPUT_H_FILE}")
    print("\n  使用方法 (ESP-IDF):")
    print("  1. 将 chinese16.h 复制到工程 main 目录")
    print("  2. 在代码中 #include \"chinese16.h\"")
    print("  3. 实现绘制像素的回调函数")
    print("  4. 调用 show_chinese_string(x, y, \"你好\", color, bg, draw_pixel, draw_ascii);")
    print("=" * 50)


if __name__ == "__main__":
    main()
