#!/usr/bin/env python3
"""
代码重构器模块
基于参考项目生成标准化代码
"""

from pathlib import Path
from typing import Dict, List, Optional
import difflib
from datetime import datetime

from ast_analyzer import FunctionInfo


class CodeRegenerator:
    """代码重新生成器"""

    def __init__(self, reference_project: str, target_project: str):
        self.reference = Path(reference_project)
        self.target = Path(target_project)
        self.output = self.target.parent / f"{self.target.name}_refactored"

    def generate_project_c(self, functions_map: Dict[str, FunctionInfo]) -> str:
        """生成版本C代码"""
        output_path = Path("output/project_c")
        output_path.mkdir(parents=True, exist_ok=True)

        reference_files = {}
        for ext in ['.c', '.h']:
            for f in self.reference.rglob(f'*{ext}'):
                if 'build' not in f.parts:
                    relative = f.relative_to(self.reference)
                    reference_files[relative] = f.read_text(encoding='utf-8')

        for file_path, content in reference_files.items():
            standardized = self._standardize_code(content, file_path)
            out_file = output_path / file_path
            out_file.parent.mkdir(parents=True, exist_ok=True)
            out_file.write_text(standardized, encoding='utf-8')

        return str(output_path)

    def _standardize_code(self, content: str, file_path: Path) -> str:
        """标准化代码格式"""
        lines = content.splitlines()
        standardized = []

        for line in lines:
            line = line.replace('\t', '    ')
            line = line.rstrip()
            standardized.append(line)

        date = datetime.now().strftime("%Y-%m-%d")
        header = f"""/**
 * @file {file_path.name}
 * @brief Auto-generated standardized version
 * @date {date}
 */
"""

        return header + '\n'.join(standardized)

    def generate_diff_patch(self, diff_blocks: List) -> str:
        """生成标准化的 diff patch 文件"""
        patch_content = []

        for block in diff_blocks:
            if block.type == 'modified':
                patch_content.append(f"--- a/{block.file_a}")
                patch_content.append(f"+++ b/{block.file_b}")
                patch_content.append(
                    f"@@ -{block.lines_a[0]},{block.lines_a[1]} +{block.lines_b[0]},{block.lines_b[1]} @@")
                patch_content.append(f" Context: {block.context}")
                patch_content.append("")

                a_lines = block.content_a.splitlines()
                b_lines = block.content_b.splitlines()

                for line in difflib.unified_diff(a_lines, b_lines, lineterm=''):
                    patch_content.append(line)

                patch_content.append("")

        return '\n'.join(patch_content)

    def suggest_standardization(self, original_code: str,
                                reference_code: str) -> Dict[str, any]:
        """基于参考代码建议标准化"""
        suggestions = {
            'original_length': len(original_code.splitlines()),
            'reference_length': len(reference_code.splitlines()),
            'style_issues': [],
            'suggested_changes': []
        }

        orig_lines = original_code.splitlines()
        ref_lines = reference_code.splitlines()

        if len(orig_lines) != len(ref_lines):
            suggestions['suggested_changes'].append({
                'type': 'line_count_mismatch',
                'message': f"行数不同: 原始 {len(orig_lines)}, 参考 {len(ref_lines)}"
            })

        for i, (orig, ref) in enumerate(zip(orig_lines, ref_lines)):
            if orig.strip() != ref.strip() and orig.strip() and ref.strip():
                if orig.strip().startswith('//') or ref.strip().startswith('//'):
                    continue
                if orig.count('\t') != ref.count('\t'):
                    suggestions['style_issues'].append({
                        'line': i + 1,
                        'issue': 'indentation_mismatch',
                        'original': orig,
                        'suggested': ref
                    })

        return suggestions