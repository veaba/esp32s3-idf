#!/usr/bin/env python3
"""
tree-sitter 语法初始化脚本
用于下载和编译 C 语言语法
"""

import subprocess
import sys
import os
import json
from pathlib import Path


def setup_tree_sitter():
    """设置 tree-sitter C 语言语法"""
    build_dir = Path("build")
    build_dir.mkdir(exist_ok=True)

    grammar_dir = Path("tree-sitter-c")

    if not grammar_dir.exists():
        print("[DOWNLOAD] Downloading tree-sitter C grammar...")
        try:
            subprocess.run(
                ["git", "clone", "--depth", "1",
                 "https://github.com/tree-sitter/tree-sitter-c"],
                check=True
            )
        except Exception as e:
            print(f"[ERROR] Git clone failed: {e}")
            print("[INFO] Trying alternative download...")
            try:
                subprocess.run(
                    ["powershell", "-Command",
                     f"Invoke-WebRequest -Uri 'https://github.com/tree-sitter/tree-sitter-c/archive/refs/heads/master.zip' -OutFile '{build_dir / 'tree-sitter-c.zip'}'"],
                    check=True
                )
                subprocess.run(
                    ["powershell", "-Command",
                     f"Expand-Archive -Path '{build_dir / 'tree-sitter-c.zip'}' -DestinationPath '{build_dir}' -Force"],
                    check=True
                )
                src_dir = build_dir / "tree-sitter-c-master"
                if src_dir.exists():
                    grammar_dir.mkdir(exist_ok=True)
                    for item in src_dir.iterdir():
                        dest = grammar_dir / item.name
                        if item.is_dir():
                            item.move_to(dest)
                        else:
                            item.move_to(dest)
                    src_dir.rmdir()
                (build_dir / "tree-sitter-c.zip").unlink()
            except Exception as e2:
                print(f"[ERROR] Alternative download also failed: {e2}")
                sys.exit(1)
    else:
        print("[OK] tree-sitter C grammar already exists")

    langs_dir = build_dir / "langs"
    langs_dir.mkdir(exist_ok=True)
    c_lib = langs_dir / "c.so"

    if not c_lib.exists():
        print("[BUILD] Building C parser library...")
        result = subprocess.run(
            ["npx", "tree-sitter", "build", str(grammar_dir), "-o", str(c_lib)],
            capture_output=True, text=True
        )
        if result.returncode != 0:
            print(f"[ERROR] Build failed: {result.stderr}")
            print("[INFO] Trying to generate parser.c...")
            subprocess.run(
                ["npx", "tree-sitter", "generate", str(grammar_dir / "src" / "grammar.json")],
                check=True
            )
            print("[INFO] Note: parser.c generated but not compiled")
            print("[INFO] tree-sitter parse will need the compiled library")
        else:
            print("[OK] Parser library built successfully")
    else:
        print("[OK] Parser library already exists")

    config_dir = Path.home() / "AppData" / "Roaming" / "tree-sitter"
    config_file = config_dir / "config.json"

    if config_file.exists():
        config = json.loads(config_file.read_text())
        if str(grammar_dir.resolve()) not in config.get("parser_directories", []):
            config["parser_directories"] = config.get("parser_directories", [])
            config["parser_directories"].append(str(grammar_dir.resolve()))
            config_file.write_text(json.dumps(config, indent=2))
            print("[OK] Updated tree-sitter config")
    else:
        print("[INFO] tree-sitter config not found")

    return c_lib


if __name__ == "__main__":
    setup_tree_sitter()