#!/usr/bin/env python3
"""
AST 解析器模块
使用简单的正则表达式和括号匹配进行 C 代码的 AST 分析
"""

from pathlib import Path
from typing import Dict, List, Optional
from dataclasses import dataclass, field
import hashlib
import re
import subprocess


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


def format_with_clang(content: str, style_config: Optional[str] = None) -> str:
    """使用 clang-format 格式化代码
    
    Args:
        content: 源代码内容
        style_config: clang-format 配置文件路径，如果为 None 则使用项目根目录的 .clang-format
    """
    clang_fmt = find_clang_format()
    if not clang_fmt:
        return content

    if style_config is None:
        style_config = str(Path(__file__).parent / ".clang-format")

    if not Path(style_config).exists():
        return content

    try:
        result = subprocess.run(
            [clang_fmt, f"--style=file:{style_config}"],
            input=content,
            capture_output=True,
            text=True,
            timeout=30
        )
        if result.returncode == 0:
            return result.stdout
    except Exception:
        pass

    return content


@dataclass
class FunctionInfo:
    """函数信息"""
    name: str
    file: str
    start_line: int
    end_line: int
    body: str
    params: List[str] = field(default_factory=list)
    return_type: str = "void"
    calls: List[str] = field(default_factory=list)
    called_by: List[str] = field(default_factory=list)
    complexity: int = 1
    hash: str = ""


class ASTAnalyzer:
    """AST 分析器 - 简单括号匹配方法"""

    def __init__(self, languages_lib=None):
        """初始化分析器"""
        pass

    def scan_functions(self, source_files: Dict[str, str]) -> Dict[str, FunctionInfo]:
        """扫描多个源文件，提取所有函数"""
        functions = {}

        for file_path, content in source_files.items():
            funcs = self._extract_functions(content, file_path)
            for func in funcs:
                key = f"{file_path}:{func.name}"
                functions[key] = func

        return functions

    def _extract_functions(self, source_code: str, file_path: str) -> List[FunctionInfo]:
        """从源代码中提取函数"""
        source_code = format_with_clang(source_code)
        functions = []
        lines = source_code.split('\n')

        i = 0
        n = len(lines)
        while i < n:
            line = lines[i].strip()

            if self._is_function_start(line):
                func_name, return_type = self._parse_function_declaration(line)
                if func_name:
                    func_start = i
                    func_body, brace_end = self._find_function_body(lines, i)
                    func_end = func_start + len(func_body) - 1 if func_body else i

                    body = '\n'.join(func_body) if func_body else ''
                    calls = self._extract_calls(body)
                    complexity = self._calculate_complexity(body)

                    func_info = FunctionInfo(
                        name=func_name,
                        file=file_path,
                        start_line=func_start + 1,
                        end_line=func_end,
                        body=body,
                        params=[],  # Simplified
                        return_type=return_type,
                        calls=calls,
                        called_by=[],
                        complexity=complexity,
                        hash=hashlib.md5(body.encode()).hexdigest()
                    )
                    functions.append(func_info)

                    if brace_end >= 0:
                        i = brace_end + 1
                        continue

            i += 1

        return functions

    def _is_function_start(self, line: str) -> bool:
        """检查是否是函数定义开始"""
        skip_words = {'if', 'while', 'for', 'switch', 'sizeof', 'return', 'case', 'default'}

        if not line or '(' not in line:
            return False

        before_paren = line.split('(')[0].strip().split()
        if not before_paren:
            return False

        last_word = before_paren[-1]
        if last_word in skip_words or last_word.startswith('_'):
            return False

        # Must have at least return_type and function_name
        if len(before_paren) < 2 and last_word not in ['main']:
            return False

        return True

    def _parse_function_declaration(self, line: str) -> tuple:
        """解析函数声明，返回 (函数名, 返回类型)"""
        paren_idx = line.index('(')
        before_paren = line[:paren_idx].strip().split()
        if len(before_paren) >= 2:
            return_type = ' '.join(before_paren[:-1])
            func_name = before_paren[-1]
            return func_name, return_type
        elif len(before_paren) == 1:
            return before_paren[0], 'int'
        return None, None

    def _find_function_body(self, lines: List[str], start: int) -> tuple:
        """找到函数体，使用括号匹配"""
        body_lines = []
        brace_count = 0
        found_first_brace = False

        for i in range(start, len(lines)):
            line = lines[i]
            body_lines.append(line)

            brace_count += line.count('{') - line.count('}')

            if not found_first_brace and '{' in line:
                found_first_brace = True

            if found_first_brace and brace_count == 0:
                return body_lines, i

        return body_lines, -1

    def _extract_calls(self, body: str) -> List[str]:
        """从函数体中提取函数调用"""
        calls = []
        skip_funcs = {'if', 'while', 'for', 'switch', 'sizeof', 'return', 'case', 'default',
                      'goto', 'break', 'continue', 'NULL', 'true', 'false', 'sizeof', 'offsetof'}

        lines = body.split('\n')
        for line in lines:
            paren_idx = line.find('(')
            if paren_idx > 0:
                before_paren = line[:paren_idx].strip().split()
                if before_paren:
                    last_word = before_paren[-1]
                    if last_word not in skip_funcs and last_word.isidentifier():
                        calls.append(last_word)

        return list(set(calls))

    def _calculate_complexity(self, body: str) -> int:
        """计算圈复杂度"""
        complexity = 1
        branching_patterns = [
            r'\bif\s*\(', r'\bwhile\s*\(', r'\bfor\s*\(',
            r'\bswitch\s*\(', r'\bcase\s+', r'\?\s*[^:]',
            r'&&', r'\|\|'
        ]

        for pattern in branching_patterns:
            complexity += len(re.findall(pattern, body))

        return complexity


def main():
    """测试用主函数"""
    analyzer = ASTAnalyzer()
    test_code = '''void app_main(void) {
    if (1) {
        printf("Hello");
    }
    while (0) {
        led_set(0);
    }
}'''
    funcs = analyzer._extract_functions(test_code, "test.c")
    print('Found {} functions'.format(len(funcs)))
    for f in funcs:
        print('  {}: complexity={}, calls={}'.format(f.name, f.complexity, f.calls))


if __name__ == '__main__':
    main()