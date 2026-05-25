#!/usr/bin/env python3
"""
VSCode Diff 集成的 AST 重排版对比工具
方案: clang-format 格式化 -> 简单解析器提取结构 -> 生成结构化输出
"""

import argparse
import subprocess
import sys
import os
import re
import hashlib
from pathlib import Path
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass, field


@dataclass
class FunctionInfo:
    name: str
    return_type: str
    params: List[str]
    body_hash: str
    calls: List[str]
    signature: str = ""
    start_line: int = 0
    body: str = ""


def find_clang_format():
    """查找 clang-format 工具"""
    candidates = [
        r"D:\programs-dev\Espressif\tools\esp-clang\esp-19.1.2_20250312\esp-clang\bin\clang-format.exe",
        "clang-format",
        "clang-format.exe",
        r"C:\Program Files\LLVM\bin\clang-format.exe",
        r"C:\Program Files (x86)\LLVM\bin\clang-format.exe",
    ]
    for candidate in candidates:
        try:
            result = subprocess.run(
                [candidate, "--version"],
                capture_output=True,
                text=True,
                timeout=5
            )
            if result.returncode == 0:
                return candidate
        except:
            continue
    return None


def format_with_clang(content: str, style: str = "llvm") -> str:
    """使用 clang-format 格式化代码"""
    clang_fmt = find_clang_format()
    if not clang_fmt:
        print("[WARN] clang-format not found, skipping formatting")
        return content

    config_path = Path(__file__).parent / ".clang-format"

    try:
        result = subprocess.run(
            [clang_fmt, f"--style=file:{config_path}"],
            input=content,
            capture_output=True,
            text=True,
            timeout=30
        )
        if result.returncode == 0:
            return result.stdout
    except Exception as e:
        print(f"[WARN] clang-format failed: {e}")

    return content


def strip_comments_and_strings(content: str) -> str:
    """去除注释但保留字符串和字符字面量（用于解析）"""
    result = []
    i = 0
    n = len(content)

    while i < n:
        if i < n - 1 and content[i] == '/' and content[i + 1] == '/':
            while i < n and content[i] != '\n':
                i += 1
            continue
        if i < n - 1 and content[i] == '/' and content[i + 1] == '*':
            i += 2
            while i < n - 1:
                if content[i] == '*' and content[i + 1] == '/':
                    i += 2
                    break
                i += 1
            continue
        result.append(content[i])
        i += 1

    return ''.join(result)


def reorder_functions(formatted_code: str, functions: List[FunctionInfo]) -> str:
    """按函数名排序重新生成代码"""
    if not functions:
        return formatted_code

    sorted_funcs = sorted(functions, key=lambda f: f.name)

    func_defs = {}
    for func in functions:
        start = func.start_line - 1
        end = start + len(func.body.split('\n'))
        func_defs[func.name] = '\n'.join(formatted_code.split('\n')[start:end])

    lines = formatted_code.split('\n')
    non_func_lines = []
    for i, line in enumerate(lines):
        is_func_line = False
        for func in functions:
            start = func.start_line - 1
            end = start + len(func.body.split('\n'))
            if start <= i < end:
                is_func_line = True
                break
        if not is_func_line:
            non_func_lines.append(line)

    non_func_code = '\n'.join(non_func_lines)

    result_lines = []
    for func in sorted_funcs:
        result_lines.append(func_defs[func.name])
        result_lines.append('')

    return non_func_code + '\n'.join(result_lines)


def find_matching_paren(line: str, start: int) -> int:
    """找到匹配的右括号位置"""
    paren_count = 0
    in_string = False
    for i in range(start, len(line)):
        c = line[i]
        if c == '"':
            in_string = not in_string
        if not in_string:
            if c == '(':
                paren_count += 1
            elif c == ')':
                paren_count -= 1
                if paren_count == 0:
                    return i
    return -1


def extract_functions(code: str) -> List[FunctionInfo]:
    """从格式化后的代码中提取函数信息"""
    functions = []
    lines = code.split('\n')

    skip_words = {
        'if', 'while', 'for', 'switch', 'sizeof', 'return', 'case', 'default',
        'typedef', 'struct', 'enum', 'union', 'goto', 'break', 'continue',
    }

    i = 0
    n = len(lines)

    while i < n:
        line = lines[i].strip()

        if not line or line.lstrip().startswith('#'):
            i += 1
            continue

        if '(' not in line:
            i += 1
            continue

        before_paren = line.split('(')[0].strip().split()
        if not before_paren:
            i += 1
            continue

        last_word = before_paren[-1]
        if last_word in skip_words:
            i += 1
            continue

        if len(before_paren) >= 2:
            return_type = ' '.join(before_paren[:-1])
            func_name = before_paren[-1]
        else:
            i += 1
            continue

        paren_end = find_matching_paren(line, line.index('('))
        if paren_end < 0:
            i += 1
            continue

        params_str = line[line.index('(') + 1:paren_end].strip()
        if params_str == 'void' or not params_str:
            params = []
        else:
            params = [p.strip() for p in params_str.split(',')]

        brace_start = -1
        for j in range(i, min(i + 10, n)):
            if '{' in lines[j]:
                brace_start = j
                break

        if brace_start < 0:
            i += 1
            continue

        brace_count = 0
        body_lines = []
        for j in range(brace_start, n):
            body_lines.append(lines[j])
            for c in lines[j]:
                if c == '{':
                    brace_count += 1
                elif c == '}':
                    brace_count -= 1
            if brace_count <= 0:
                break

        body = '\n'.join(body_lines)
        body_hash = hash_code(body)
        calls = extract_calls(body)
        calls = [c for c in calls if c != func_name]

        signature = f"{return_type} {func_name}({', '.join(params)})"

        func_info = FunctionInfo(
            name=func_name,
            return_type=return_type,
            params=params,
            body_hash=body_hash,
            calls=sorted(set(calls)),
            signature=signature,
            start_line=brace_start + 1,
            body=body
        )
        functions.append(func_info)

        i = brace_start + len(body_lines)
        while i < n and not lines[i].strip():
            i += 1

    return functions


def extract_calls(body: str) -> List[str]:
    """从函数体中提取函数调用"""
    calls = []
    skip_funcs = {
        'if', 'while', 'for', 'switch', 'sizeof', 'return', 'case', 'default',
        'goto', 'break', 'continue', 'NULL', 'true', 'false', 'offsetof',
        'printf', 'print', 'puts', 'putchar', 'exit', 'abort', 'memcpy', 'memset',
        'strlen', 'strcpy', 'strncpy', 'strcmp', 'strncmp', 'atoi', 'atof',
        'malloc', 'free', 'calloc', 'realloc', 'vPortMalloc', 'free', 'pvPortMalloc',
    }

    lines = body.split('\n')
    for line in lines:
        line = line.strip()
        if not line or line.startswith('//') or line.startswith('/*'):
            continue

        if '(' in line:
            before_paren = line.split('(')[0].strip().split()
            if before_paren:
                last_word = before_paren[-1]
                if last_word not in skip_funcs and last_word.isidentifier():
                    calls.append(last_word)

    return calls


def hash_code(text: str) -> str:
    """生成代码的哈希值"""
    normalized = re.sub(r'\s+', ' ', text.strip())
    return hashlib.md5(normalized.encode()).hexdigest()[:12]


def trim_leading_empty_lines(content: str) -> str:
    """删除文件开头的连续空行"""
    lines = content.split('\n')
    start = 0
    for i, line in enumerate(lines):
        if line.strip() == '':
            start = i + 1
        else:
            break
    return '\n'.join(lines[start:])


def vscode_diff(file_a: str, file_b: str):
    """使用 VSCode 打开 diff"""
    if sys.platform == 'win32':
        vscode_paths = [
            r"D:\Program Files (x86)\Microsoft VS Code\bin\code.cmd",
            r"D:\Program Files\Microsoft VS Code\bin\code.cmd",
            r"C:\Program Files (x86)\Microsoft VS Code\bin\code.cmd",
            r"C:\Program Files\Microsoft VS Code\bin\code.cmd",
        ]
        for path in vscode_paths:
            if os.path.exists(path):
                cmd = f'"{path}" --diff "{file_a}" "{file_b}"'
                subprocess.Popen(cmd, shell=True)
                print("[OK] Opened VSCode diff")
                return

        print("[WARN] VSCode not found in standard locations")
    else:
        subprocess.Popen(['code', '--diff', file_a, file_b])
        print("[OK] Opened VSCode diff")


def process_file(content: str) -> Tuple[str, List[FunctionInfo]]:
    """处理单个文件: 格式化 + 提取函数"""
    stripped = strip_comments_and_strings(content)
    formatted = format_with_clang(stripped)
    formatted = trim_leading_empty_lines(formatted)
    functions = extract_functions(formatted)
    reordered = reorder_functions(formatted, functions)
    return reordered, functions


def generate_json_output(functions: List[FunctionInfo], file_name: str) -> str:
    """生成 JSON 格式的结构化输出"""
    import json
    return json.dumps({
        "file": file_name,
        "functions": [asdict(f) for f in functions]
    }, indent=2)


def generate_comparison_output(funcs_a: List[FunctionInfo], funcs_b: List[FunctionInfo], name_a: str, name_b: str) -> str:
    """生成比较输出（文本格式）"""
    lines = []
    lines.append("=" * 60)
    lines.append("AST-based Code Comparison")
    lines.append("=" * 60)
    lines.append("")

    dict_a = {f.name: f for f in funcs_a}
    dict_b = {f.name: f for f in funcs_b}

    all_names = sorted(set(dict_a.keys()) | set(dict_b.keys()))

    match_count = 0
    diff_count = 0
    only_a_count = 0
    only_b_count = 0

    lines.append("FUNCTIONS:")
    lines.append("-" * 40)

    for name in all_names:
        in_a = name in dict_a
        in_b = name in dict_b

        if in_a and in_b:
            fa = dict_a[name]
            fb = dict_b[name]
            if fa.body_hash == fb.body_hash:
                lines.append(f"  [MATCH] {fa.signature}")
                match_count += 1
            else:
                lines.append(f"  [DIFF]  {fa.signature}")
                diff_count += 1
                if fa.calls != fb.calls:
                    lines.append(f"         A calls: {fa.calls}")
                    lines.append(f"         B calls: {fb.calls}")
                if fa.params != fb.params:
                    lines.append(f"         A params: {fa.params}")
                    lines.append(f"         B params: {fb.params}")
        elif in_a:
            lines.append(f"  [A ONLY] {dict_a[name].signature}")
            only_a_count += 1
        else:
            lines.append(f"  [B ONLY] {dict_b[name].signature}")
            only_b_count += 1

    lines.append("")
    lines.append(f"Summary: {match_count} match, {diff_count} different, {only_a_count} only in A, {only_b_count} only in B")

    return '\n'.join(lines)


def asdict(obj):
    """将 dataclass 转换为字典（包括嵌套的 dataclass）"""
    if hasattr(obj, '__dataclass_fields__'):
        result = {}
        for field_name in obj.__dataclass_fields__:
            value = getattr(obj, field_name)
            if isinstance(value, list):
                result[field_name] = [asdict(v) if hasattr(v, '__dataclass_fields__') else v for v in value]
            elif hasattr(value, '__dataclass_fields__'):
                result[field_name] = asdict(value)
            else:
                result[field_name] = value
        return result
    return obj


def main():
    parser = argparse.ArgumentParser(
        description='AST-based code comparison with VSCode diff integration',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  %(prog)s file_a.c file_b.c                    # Compare two files
  %(prog)s app/main.c my/main.c -o ./output      # Custom output dir
  %(prog)s ref/main.c my/main.c --no-open       # Generate only, no VSCode open
  %(prog)s --help                              # Show this help
        '''
    )
    parser.add_argument('file_a', help='First file to compare (your project)')
    parser.add_argument(
        'file_b', help='Second file to compare (reference project)')
    parser.add_argument('--output', '-o', default='./diff_output',
                        help='Output directory for formatted files (default: ./diff_output)')
    parser.add_argument('--no-open', action='store_true',
                        help='Do not open VSCode diff (only generate formatted files)')
    parser.add_argument('--keep', action='store_true',
                        help='Keep temporary formatted files after comparison')
    parser.add_argument('--style', default='llvm',
                        help='clang-format style (llvm, google, chromium, mozilla)')
    parser.add_argument('--json', action='store_true',
                        help='Output comparison as JSON')

    args = parser.parse_args()

    if not Path(args.file_a).exists():
        print(f"[ERROR] File A does not exist: {args.file_a}")
        sys.exit(1)
    if not Path(args.file_b).exists():
        print(f"[ERROR] File B does not exist: {args.file_b}")
        sys.exit(1)

    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)

    print("[1/4] Reading files...")
    with open(args.file_a, 'r', encoding='utf-8', errors='ignore') as f:
        content_a = f.read()
    with open(args.file_b, 'r', encoding='utf-8', errors='ignore') as f:
        content_b = f.read()

    print("[2/4] Formatting with clang-format...")
    formatted_a, funcs_a = process_file(content_a)
    formatted_b, funcs_b = process_file(content_b)

    name_a = Path(args.file_a).name
    name_b = Path(args.file_b).name

    print("[3/4] Writing formatted files...")
    output_a = output_dir / "A___{}".format(name_a)
    output_b = output_dir / "B___{}".format(name_b)

    with open(output_a, 'w', encoding='utf-8') as f:
        f.write(formatted_a)
    with open(output_b, 'w', encoding='utf-8') as f:
        f.write(formatted_b)

    print("[4/4] Generating comparison report...")
    comparison = generate_comparison_output(funcs_a, funcs_b, name_a, name_b)
    report_path = output_dir / "comparison_report.txt"
    with open(report_path, 'w', encoding='utf-8') as f:
        f.write(comparison)
    print(f"       Report: {report_path}")

    json_output_a = output_dir / "A___{}.json".format(name_a)
    json_output_b = output_dir / "B___{}.json".format(name_b)
    with open(json_output_a, 'w', encoding='utf-8') as f:
        f.write(generate_json_output(funcs_a, name_a))
    with open(json_output_b, 'w', encoding='utf-8') as f:
        f.write(generate_json_output(funcs_b, name_b))

    print("")
    print("Formatted files:")
    print(f"  A: {output_a}")
    print(f"  B: {output_b}")

    if not args.no_open:
        vscode_diff(str(output_a), str(output_b))

    if not args.keep:
        pass

    print("")
    print(comparison)


if __name__ == '__main__':
    main()