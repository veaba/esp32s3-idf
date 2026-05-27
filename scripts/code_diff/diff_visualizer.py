#!/usr/bin/env python3
"""
差异可视化模块
"""

from typing import Dict, List, Any
import difflib
from dataclasses import dataclass
from pathlib import Path


@dataclass
class DiffBlock:
    """差异块"""
    type: str
    file_a: str
    file_b: str
    lines_a: tuple
    lines_b: tuple
    content_a: str
    content_b: str
    context: str = ""


def generate_unified_diff(content_a: str, content_b: str,
                          fromfile: str = "a", tofile: str = "b") -> str:
    """生成 unified diff 格式的差异"""
    diff_lines = list(difflib.unified_diff(
        content_a.splitlines(keepends=True),
        content_b.splitlines(keepends=True),
        fromfile=fromfile,
        tofile=tofile,
        lineterm=''
    ))
    return '\n'.join(diff_lines)


def format_diff_text(diffs: List[Dict], verbose: bool = False) -> str:
    """格式化差异为文本"""
    output = []
    for i, diff in enumerate(diffs, 1):
        output.append(f"\n{'=' * 60}")
        output.append(f"[{i}] {diff.get('type', 'unknown')}")

        if 'function' in diff:
            output.append(f"  函数: {diff['function']}")
        if 'file_a' in diff:
            output.append(f"  文件A: {diff['file_a']}")
        if 'file_b' in diff:
            output.append(f"  文件B: {diff['file_b']}")

        if verbose and 'diff' in diff:
            output.append(f"\n{diff['diff']}")

        if 'suggestion' in diff:
            output.append(f"\n  建议: {diff['suggestion']}")

    return '\n'.join(output)


def format_diff_table(diffs: List[Dict]) -> str:
    """格式化差异为表格"""
    rows = []
    for diff in diffs:
        diff_type = diff.get('type', 'unknown')
        func_name = diff.get('function', 'N/A')
        suggestion = diff.get('suggestion', '')[:50]
        rows.append(f"| {diff_type:<25} | {func_name:<30} | {suggestion} |")

    header = "| 类型 | 函数 | 建议 |"
    separator = "|-" + "-" * 25 + "-+-" + "-" * 30 + "-+-" + "-" * 40 + "-|"

    return f"{header}\n{separator}\n" + '\n'.join(rows)


def generate_html_report(report: Dict) -> str:
    """生成 HTML 可视化报告"""
    summary = report.get('summary', {})
    differences = report.get('differences', [])

    html = f"""<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>ESP32 代码对比报告</title>
    <style>
        * {{ margin: 0; padding: 0; box-sizing: border-box; }}
        body {{ font-family: 'Segoe UI', Arial, sans-serif; background: #1a1a2e; color: #eee; padding: 20px; }}
        .container {{ max-width: 1200px; margin: 0 auto; }}
        h1 {{ color: #00d9ff; text-align: center; margin-bottom: 30px; }}
        .summary {{ background: #16213e; border-radius: 10px; padding: 20px; margin-bottom: 20px; }}
        .summary-grid {{ display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; }}
        .stat {{ background: #0f3460; padding: 15px; border-radius: 8px; text-align: center; }}
        .stat-value {{ font-size: 2em; color: #00d9ff; font-weight: bold; }}
        .stat-label {{ color: #888; margin-top: 5px; }}
        .diffs {{ margin-top: 20px; }}
        .diff-item {{ background: #16213e; border-radius: 10px; margin-bottom: 15px; overflow: hidden; }}
        .diff-header {{ background: #0f3460; padding: 15px; display: flex; justify-content: space-between; align-items: center; }}
        .diff-type {{ color: #e94560; font-weight: bold; font-size: 1.1em; }}
        .diff-function {{ color: #00d9ff; font-size: 1.2em; }}
        .diff-body {{ padding: 15px; }}
        .diff-content {{ background: #0a0a1a; padding: 10px; border-radius: 5px; overflow-x: auto; }}
        .diff-content pre {{ color: #aaa; font-family: 'Consolas', monospace; font-size: 0.9em; white-space: pre-wrap; }}
        .suggestion {{ background: #1a3a1a; border-left: 4px solid #4caf50; padding: 10px; margin-top: 10px; color: #4caf50; }}
        .add {{ color: #4caf50; }}
        .remove {{ color: #e94560; }}
        .stats-row {{ display: flex; gap: 10px; margin-top: 10px; }}
        .stat-chip {{ background: #0f3460; padding: 5px 10px; border-radius: 15px; font-size: 0.8em; }}
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32 代码对比分析报告</h1>

        <div class="summary">
            <div class="summary-grid">
                <div class="stat">
                    <div class="stat-value">{summary.get('total_functions_a', 0)}</div>
                    <div class="stat-label">项目A 函数数</div>
                </div>
                <div class="stat">
                    <div class="stat-value">{summary.get('total_functions_b', 0)}</div>
                    <div class="stat-label">项目B 函数数</div>
                </div>
                <div class="stat">
                    <div class="stat-value">{summary.get('total_differences', 0)}</div>
                    <div class="stat-label">发现差异</div>
                </div>
            </div>
            <div class="stats-row" style="margin-top: 15px;">
"""

    for diff_type, count in summary.get('type_breakdown', {}).items():
        html += f'<span class="stat-chip">{diff_type}: {count}</span>'

    html += """
            </div>
        </div>

        <div class="diffs">
"""

    for diff in differences:
        diff_type = diff.get('type', 'unknown')
        func_name = diff.get('function', 'N/A')
        file_a = diff.get('file_a', '')
        file_b = diff.get('file_b', '')
        suggestion = diff.get('suggestion', '')
        diff_text = diff.get('diff', '')

        html += f"""
            <div class="diff-item">
                <div class="diff-header">
                    <span class="diff-type">{diff_type}</span>
                    <span class="diff-function">{func_name}</span>
                </div>
                <div class="diff-body">
"""

        if file_a or file_b:
            html += f"<p style='color:#888;margin-bottom:10px;'>A: {file_a} → B: {file_b}</p>"

        if diff_text:
            escaped_diff = diff_text.replace('<', '&lt;').replace('>', '&gt;')
            html += f'<div class="diff-content"><pre>{escaped_diff}</pre></div>'

        if suggestion:
            html += f'<div class="suggestion">Suggestion: {suggestion}</div>'

        html += """
                </div>
            </div>
"""

    html += """
        </div>
    </div>
</body>
</html>
"""
    return html


def main():
    """CLI 入口"""
    import argparse
    import json

    parser = argparse.ArgumentParser(description='差异可视化工具')
    parser.add_argument('report', help='JSON 报告文件路径')
    parser.add_argument('--output', '-o', default='report.html', help='输出 HTML 文件')
    args = parser.parse_args()

    with open(args.report, 'r', encoding='utf-8') as f:
        report = json.load(f)

    html = generate_html_report(report)
    Path(args.output).write_text(html, encoding='utf-8')
    print(f"HTML 报告已生成: {args.output}")


if __name__ == '__main__':
    main()