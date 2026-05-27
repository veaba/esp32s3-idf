#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
读取 chinese_3k_unicode.txt，每16个字符一行输出，字符之间有空格 + unicode。
"""

INPUT_FILE = "chinese_3k_unicode.txt"
OUTPUT_FILE = "chinese_3k.txt"

with open(INPUT_FILE, "r", encoding="utf-8") as f:
    text = f.read()

# 去掉空白字符（空格、换行、制表符等），只保留可见字符
chars = text.replace("\n", "").replace(" ", "").replace("\t", "")

lines = []
for i in range(0, len(chars), 16):
    chinese_str = chars[i:i+16]
    chinese_space = " ".join(chinese_str)  # add space

    # convert to hex unicode
    chinese_unicode = " ".join([f"{ord(c):04X}" for c in chinese_str])

    lines.append(chinese_space)
    lines.append(chinese_unicode)

output = "\n".join(lines)

with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
    f.write(output)

print(f"输入字符总数: {len(chars)}")
print(f"输出行数: {len(lines)}")
print(f"已写入: {OUTPUT_FILE}")
