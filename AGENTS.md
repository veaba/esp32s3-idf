# esp32s3-idf

## Projects

- `code_diff/` — Python tool for AST-based ESP-IDF code comparison
- `01_led` through `10_spi_lcd/` — ESP-IDF example projects (C, CMake)

## code_diff — Python tool

**Package manager**: `uv` (not pip/poetry)

**Run commands**:
```bash
uv run esp32-diff <ref_project> <your_project>   # full project comparison
uv run esp32-diff <ref> <yours> -f json -o ./report
uv run esp32-diff <ref> <yours> -r                # regenerate
uv run vscode-diff <file_a> <file_b>              # single file diff
```

**Setup** (first time): `python setup_tree_sitter.py` — clones tree-sitter-c grammar and builds `build/langs/c.so` via `npx tree-sitter build`

**Dev tools**: `ruff`, `black` (line-length 100), `mypy`, `pytest`

**Entry points** (`pyproject.toml` scripts): `esp32-diff` → `comparator:cli_main`, `vscode-diff` → `vscode_diff:main`

**Python**: 3.10–3.12, requires `tree-sitter==0.22.0` (locked to >=0.20.0 via `[tool.uv]`)

**Windows**: comparator.py wraps stdout/stderr with UTF-8 encoding for Chinese output