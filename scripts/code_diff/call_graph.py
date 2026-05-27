#!/usr/bin/env python3
"""
函数调用图分析模块
"""

from typing import Dict, List, Set, Tuple
from collections import defaultdict
from dataclasses import dataclass, field

from ast_analyzer import FunctionInfo


@dataclass
class CallGraphNode:
    """调用图节点"""
    key: str
    func_info: FunctionInfo
    callees: List[str] = field(default_factory=list)
    callers: List[str] = field(default_factory=list)


class CallGraphBuilder:
    """调用图构建器"""

    def __init__(self):
        self.graph: Dict[str, CallGraphNode] = {}

    def build(self, functions: Dict[str, FunctionInfo]) -> Dict[str, CallGraphNode]:
        """从函数集合构建调用图"""
        self.graph = {}

        for key, func_info in functions.items():
            if key not in self.graph:
                self.graph[key] = CallGraphNode(key=key, func_info=func_info)

        for key, func_info in functions.items():
            for called_func in func_info.calls:
                self._link_functions(key, called_func, functions)

        return self.graph

    def _link_functions(self, caller_key: str, called_name: str,
                        functions: Dict[str, FunctionInfo]):
        """链接调用关系"""
        found = False
        for target_key, target_func in functions.items():
            if target_func.name == called_name:
                self.graph[caller_key].callees.append(target_key)
                self.graph[target_key].callers.append(caller_key)
                found = True
                break

        if not found:
            self.graph[caller_key].callees.append(f"external:{called_name}")

    def get_callers(self, func_key: str) -> List[str]:
        """获取调用指定函数的所有函数"""
        if func_key in self.graph:
            return self.graph[func_key].callers
        return []

    def get_callees(self, func_key: str) -> List[str]:
        """获取指定函数调用的所有函数"""
        if func_key in self.graph:
            return self.graph[func_key].callees
        return []

    def find_top_callers(self, n: int = 5) -> List[Tuple[str, int]]:
        """获取被调用次数最多的函数"""
        call_counts = []
        for key, node in self.graph.items():
            call_counts.append((key, len(node.callers)))

        return sorted(call_counts, key=lambda x: x[1], reverse=True)[:n]

    def find_leaf_functions(self) -> List[str]:
        """找出叶子函数（不调用其他函数的函数）"""
        leaf_funcs = []
        for key, node in self.graph.items():
            if not node.callees or all(c.startswith("external:") for c in node.callees):
                leaf_funcs.append(key)
        return leaf_funcs

    def find_entry_points(self) -> List[str]:
        """找出入口函数（不被其他函数调用的函数）"""
        entry_points = []
        for key, node in self.graph.items():
            if not node.callers:
                entry_points.append(key)
        return entry_points


def compare_call_graphs(graph_a: Dict[str, CallGraphNode],
                        graph_b: Dict[str, CallGraphNode]) -> Dict:
    """对比两个调用图"""
    diff_result = {
        'nodes_a': len(graph_a),
        'nodes_b': len(graph_b),
        'edges_a': sum(len(n.callees) for n in graph_a.values()),
        'edges_b': sum(len(n.callees) for n in graph_b.values()),
        'only_in_a': [],
        'only_in_b': [],
        'different_calls': []
    }

    keys_a = set(graph_a.keys())
    keys_b = set(graph_b.keys())

    diff_result['only_in_a'] = list(keys_a - keys_b)
    diff_result['only_in_b'] = list(keys_b - keys_a)

    common_keys = keys_a & keys_b
    for key in common_keys:
        callees_a = set(graph_a[key].callees)
        callees_b = set(graph_b[key].callees)
        if callees_a != callees_b:
            diff_result['different_calls'].append({
                'function': key,
                'callees_a': list(callees_a),
                'callees_b': list(callees_b)
            })

    return diff_result