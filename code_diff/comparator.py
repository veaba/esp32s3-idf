#!/usr/bin/env python3
"""
ESP32 项目代码深度对比与重构工具
基于 AST 的智能代码对比与重构
"""

import sys
import io
from pathlib import Path
from typing import Annotated, Dict, List
import difflib
from dataclasses import dataclass, field
from collections import defaultdict

if sys.platform == 'win32':
    try:
        sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
        sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8', errors='replace')
    except Exception:
        pass

try:
    import typer
    from rich.console import Console
    from rich.progress import Progress, SpinnerColumn, TextColumn
    from rich.panel import Panel
    from rich import print as rprint
    RICH_AVAILABLE = True
except ImportError:
    RICH_AVAILABLE = False
    print("Tip: Install rich and typer for better terminal experience")
    print("Run: uv pip install rich typer")

from ast_analyzer import ASTAnalyzer, FunctionInfo
from call_graph import CallGraphBuilder, CallGraphNode
from code_regenerator import CodeRegenerator
from diff_visualizer import generate_html_report


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


if RICH_AVAILABLE:
    app = typer.Typer(name="esp32-diff", help="ESP32 项目代码深度对比与重构工具", add_completion=False)
    console = Console()


def check_dependencies():
    """检查必需的依赖"""
    try:
        import tree_sitter
    except ImportError:
        print("[ERROR] tree-sitter not installed")
        sys.exit(1)


def load_config() -> dict:
    """加载配置文件"""
    config_path = Path("config.yaml")
    if config_path.exists():
        import yaml
        with open(config_path, encoding='utf-8') as f:
            return yaml.safe_load(f).get('comparison', {})
    return {
        'source_extensions': ['.c', '.h'],
        'ignore_patterns': ['build', 'managed_components'],
    }


def scan_project(project_path: Path, config: dict) -> Dict[str, str]:
    """扫描项目中的所有源文件"""
    source_files = {}
    extensions = config.get('source_extensions', ['.c', '.h'])

    for ext in extensions:
        for file_path in project_path.rglob(f'*{ext}'):
            if 'build' in file_path.parts:
                continue
            relative_path = file_path.relative_to(project_path)
            try:
                source_files[str(relative_path)] = file_path.read_text(encoding='utf-8', errors='ignore')
            except Exception:
                continue

    return source_files


class ProjectComparator:
    """项目对比器核心"""

    def __init__(self, project_a: str, project_b: str, config: dict):
        self.project_a = Path(project_a)
        self.project_b = Path(project_b)
        self.config = config
        self.analyzer = self._init_analyzer()
        self.functions_a: Dict[str, FunctionInfo] = {}
        self.functions_b: Dict[str, FunctionInfo] = {}
        self.call_graph_a: Dict[str, CallGraphNode] = {}
        self.call_graph_b: Dict[str, CallGraphNode] = {}
        self.diffs: List[DiffBlock] = []

    def _init_analyzer(self) -> ASTAnalyzer:
        """初始化 AST 分析器"""
        return ASTAnalyzer()

    def run_full_analysis(self) -> Dict:
        """运行完整分析"""
        if RICH_AVAILABLE:
            console.print(Panel.fit(
                "[bold cyan]ESP32 Code Deep Comparison Tool[/bold cyan]\n"
                "[dim]AST-based intelligent code comparison and refactoring[/dim]",
                border_style="cyan"
            ))

        print("=" * 60)
        print("ESP32 Code Deep Comparison Analysis")
        print("=" * 60)

        source_a = scan_project(self.project_a, self.config)
        source_b = scan_project(self.project_b, self.config)
        print("\n[1/4] Scanning project source files...")
        print("  Project A: {} source files".format(len(source_a)))
        print("  Project B: {} source files".format(len(source_b)))

        print("\n[2/4] AST parsing - extracting function info...")
        self.functions_a = self.analyzer.scan_functions(source_a)
        self.functions_b = self.analyzer.scan_functions(source_b)
        print("  Project A: {} functions".format(len(self.functions_a)))
        print("  Project B: {} functions".format(len(self.functions_b)))

        print("\n[3/4] Building function call graph...")
        graph_builder = CallGraphBuilder()
        self.call_graph_a = graph_builder.build(self.functions_a)
        self.call_graph_b = graph_builder.build(self.functions_b)

        print("\n[4/4] Deep comparison analysis...")
        differences = self.compare_ast()
        print("  Found {} differences".format(len(differences)))

        return self.generate_report(differences)

    def compare_ast(self) -> List[Dict]:
        """对比两个项目的 AST"""
        differences = []

        func_names_a = {func.name for func in self.functions_a.values()}
        func_names_b = {func.name for func in self.functions_b.values()}

        only_in_a = func_names_a - func_names_b
        for name in only_in_a:
            func_info = next((f for f in self.functions_a.values() if f.name == name), None)
            differences.append({
                'type': 'function_only_in_a',
                'function': name,
                'file': func_info.file if func_info else 'unknown',
                'suggestion': f'函数 {name} 仅在项目A中存在，检查是否遗漏'
            })

        only_in_b = func_names_b - func_names_a
        for name in only_in_b:
            func_info = next((f for f in self.functions_b.values() if f.name == name), None)
            differences.append({
                'type': 'function_only_in_b',
                'function': name,
                'file': func_info.file if func_info else 'unknown',
                'suggestion': f'函数 {name} 仅在项目B中存在，检查是否多余添加'
            })

        common_names = func_names_a & func_names_b
        for name in common_names:
            for key_a, func_a in self.functions_a.items():
                if func_a.name == name:
                    for key_b, func_b in self.functions_b.items():
                        if func_b.name == name:
                            if func_a.hash != func_b.hash:
                                diff_text = self._generate_function_diff(func_a, func_b)
                                differences.append({
                                    'type': 'function_modified',
                                    'function': name,
                                    'file_a': func_a.file,
                                    'file_b': func_b.file,
                                    'hash_a': func_a.hash,
                                    'hash_b': func_b.hash,
                                    'complexity_a': func_a.complexity,
                                    'complexity_b': func_b.complexity,
                                    'diff': diff_text,
                                    'suggestion': self._suggest_fix(func_a, func_b)
                                })

        return differences

    def _generate_function_diff(self, func_a: FunctionInfo, func_b: FunctionInfo) -> str:
        """生成两个函数的文本差异"""
        diff_lines = list(difflib.unified_diff(
            func_a.body.splitlines(keepends=True),
            func_b.body.splitlines(keepends=True),
            fromfile=f'A/{func_a.file}:{func_a.name}',
            tofile=f'B/{func_b.file}:{func_b.name}',
            lineterm=''
        ))
        return '\n'.join(diff_lines)

    def _suggest_fix(self, func_a: FunctionInfo, func_b: FunctionInfo) -> str:
        """基于差异给出修复建议"""
        suggestions = []

        if abs(func_a.complexity - func_b.complexity) > 2:
            suggestions.append(
                f"圈复杂度差异较大 (A:{func_a.complexity}, B:{func_b.complexity})，检查是否缺少条件分支"
            )

        if func_a.params != func_b.params:
            suggestions.append(f"参数列表不同: A有 {len(func_a.params)} 个参数，B有 {len(func_b.params)} 个参数")

        calls_a = set(func_a.calls)
        calls_b = set(func_b.calls)
        missing_calls = calls_a - calls_b
        extra_calls = calls_b - calls_a

        if missing_calls:
            suggestions.append(f"B 中缺少函数调用: {', '.join(missing_calls)}")
        if extra_calls:
            suggestions.append(f"B 中多余函数调用: {', '.join(extra_calls)}")

        return '\n'.join(suggestions) if suggestions else "无明显结构性差异，请检查变量值和初始化"

    def generate_report(self, differences: List[Dict]) -> Dict:
        """生成分析报告"""
        graph_builder = CallGraphBuilder()
        return {
            'project_a': str(self.project_a),
            'project_b': str(self.project_b),
            'summary': {
                'total_functions_a': len(self.functions_a),
                'total_functions_b': len(self.functions_b),
                'total_differences': len(differences),
                'type_breakdown': self._categorize_differences(differences)
            },
            'call_graph_comparison': {
                'a_edges': sum(len(n.callees) for n in self.call_graph_a.values()),
                'b_edges': sum(len(n.callees) for n in self.call_graph_b.values())
            },
            'differences': differences,
            'top_callers': {
                'project_a': graph_builder.find_top_callers(5),
                'project_b': graph_builder.find_top_callers(5)
            }
        }

    def _categorize_differences(self, differences: List[Dict]) -> Dict:
        """按类型分类差异"""
        categories = defaultdict(int)
        for diff in differences:
            categories[diff['type']] += 1
        return dict(categories)


@app.command()
def main(
    project_a: str,
    project_b: str,
    output: str = "./comparison_report",
    format: str = "json",
    regenerate: bool = False,
    verbose: bool = False,
):
    """ESP32 项目代码深度对比与重构工具"""
    check_dependencies()

    if not project_a or not Path(project_a).exists():
        typer.echo(f"[ERROR] Project A path does not exist: {project_a}", err=True)
        raise typer.Exit(code=1)
    if not project_b or not Path(project_b).exists():
        typer.echo(f"[ERROR] Project B path does not exist: {project_b}", err=True)
        raise typer.Exit(code=1)

    config = load_config()

    comparator = ProjectComparator(project_a, project_b, config)
    report = comparator.run_full_analysis()

    output_dir = Path(output)
    output_dir.mkdir(parents=True, exist_ok=True)

    if format == 'json':
        import json
        report_file = output_dir / 'comparison_report.json'
        with open(report_file, 'w', encoding='utf-8') as f:
            json.dump(report, f, indent=2, ensure_ascii=False)
        print(f"\n[OK] JSON report saved to: {report_file}")

    elif format == 'html':
        html_content = generate_html_report(report)
        report_file = output_dir / 'comparison_report.html'
        with open(report_file, 'w', encoding='utf-8') as f:
            f.write(html_content)
        print(f"\n[OK] HTML report saved to: {report_file}")

    else:
        print_text_report(report)

    if regenerate:
        print("\n[WORKING] Generating refactored C code...")
        regenerator = CodeRegenerator(project_a, project_b)
        output_path = regenerator.generate_project_c(comparator.functions_a)
        print(f"[OK] Refactored C code generated at: {output_path}")


def print_text_report(report: Dict):
    """打印文本格式报告"""
    print("\n" + "=" * 60)
    print("Comparison Analysis Results")
    print("=" * 60)

    print("\n[INFO] Project A: {}".format(report['project_a']))
    print("[INFO] Project B: {}".format(report['project_b']))
    print("\n[STAT] Function stats:")
    print("  A: {} functions".format(report['summary']['total_functions_a']))
    print("  B: {} functions".format(report['summary']['total_functions_b']))
    print("  [!] Differences: {}".format(report['summary']['total_differences']))

    print("\n[BREAKDOWN] Differences by type:")
    for diff_type, count in report['summary']['type_breakdown'].items():
        print("  - {}: {}".format(diff_type, count))

    print("\n[KEY DIFFERENCES]:")
    for i, diff in enumerate(report['differences'][:10], 1):
        print("\n  [{}] {}".format(i, diff['type']))
        if 'function' in diff:
            print("      Function: {}".format(diff['function']))
        if 'suggestion' in diff:
            print("      Suggestion: {}".format(diff['suggestion']))


def cli_main():
    import argparse

    parser = argparse.ArgumentParser(description='ESP32 Code Deep Comparison Tool')
    parser.add_argument('project_a', help='Reference project path')
    parser.add_argument('project_b', help='Target project path')
    parser.add_argument('--output', '-o', default='./comparison_report')
    parser.add_argument('--format', '-f', choices=['json', 'html', 'text'], default='json')
    parser.add_argument('--regenerate', '-r', action='store_true')
    parser.add_argument('--verbose', '-v', action='store_true')

    args = parser.parse_args()

    check_dependencies()

    if not Path(args.project_a).exists():
        print("[ERROR] Project A path does not exist: {}".format(args.project_a))
        sys.exit(1)
    if not Path(args.project_b).exists():
        print("[ERROR] Project B path does not exist: {}".format(args.project_b))
        sys.exit(1)

    config = load_config()
    comparator = ProjectComparator(args.project_a, args.project_b, config)
    report = comparator.run_full_analysis()

    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)

    if args.format == 'json':
        import json
        report_file = output_dir / 'comparison_report.json'
        with open(report_file, 'w', encoding='utf-8') as f:
            json.dump(report, f, indent=2, ensure_ascii=False)
        print("\n[OK] JSON report saved to: {}".format(report_file))

    elif args.format == 'html':
        html_content = generate_html_report(report)
        report_file = output_dir / 'comparison_report.html'
        with open(report_file, 'w', encoding='utf-8') as f:
            f.write(html_content)
        print("\n[OK] HTML report saved to: {}".format(report_file))

    else:
        print_text_report(report)

    if args.regenerate:
        print("\n[WORKING] Generating refactored C code...")
        regenerator = CodeRegenerator(args.project_a, args.project_b)
        output_path = regenerator.generate_project_c(comparator.functions_a)
        print("[OK] Refactored C code generated at: {}".format(output_path))


if __name__ == '__main__':
    cli_main()