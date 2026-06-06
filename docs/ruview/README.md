# AGENTS.md — RuView (wifi-densepose)

## Project Structure

Dual codebase: **Python v1** at `archive/v1/`, **Rust v2** at `v2/`. The Rust workspace is the active codebase.

```
v2/crates/               # Rust workspace (22 crates)
  wifi-densepose-core/   # Core types, traits, error types (leaf, no internal deps)
  wifi-densepose-signal/  # Signal processing + RuvSense (depends on core, ruvector)
  wifi-densepose-nn/      # Neural network inference (no internal deps)
  wifi-densepose-ruvector/# Cross-viewpoint fusion (no internal deps)
  wifi-densepose-mat/     # Mass Casualty Assessment (depends on core, signal, nn)
  wifi-densepose-train/   # Training pipeline (depends on signal, nn)
  wifi-densepose-api/     # REST API — Axum (no internal deps)
  wifi-densepose-cli/     # CLI binary (depends on mat)
  wifi-densepose-sensing-server/ # Lightweight Axum server for live UI (depends on wifiscan)
  wifi-densepose-desktop/ # Tauri desktop app
  wifi-densepose-pointcloud/ # 3D point cloud
  wifi-densepose-geo/     # Geospatial integration
  wifi-densepose-wasm/    # Browser WASM bindings (depends on mat)
  wifi-densepose-wasm-edge/ # ESP32 WASM edge — EXCLUDED from workspace
  nvsim/                  # NV-diamond simulator (standalone leaf, WASM-ready)
  nvsim-server/           # NV-diamond web dashboard
  ruv-neural/             # RuVector neural sub-crate cluster
firmware/esp32-csi-node/  # ESP32-S3 C firmware (CSI capture, TDM, NVS config)
ui/                       # Web UI (JS, no build step — served by sensing-server or python -m http.server)
dashboard/                # NVSim dashboard (Vite + Lit + TypeScript, npm-based)
archive/v1/              # Legacy Python pipeline (proof-of-concept)
scripts/                 # CLI tools: rf-scan.js, train-wiflow.js, qemu scripts, etc.
docs/adr/                # 90+ Architecture Decision Records
```

## Build & Test Commands

### Rust (primary)
```bash
# Full workspace test — MUST use --no-default-features (BLAS/eigenvalue may not be available)
cd v2
cargo test --workspace --no-default-features

# Single crate check (no GPU needed)
cargo check -p wifi-densepose-train --no-default-features

# Benchmarks
cargo bench --package wifi-densepose-signal

# Build sensing server (for live UI)
cargo build -p wifi-densepose-sensing-server --no-default-features
```

### WASM edge crate — special build
`wifi-densepose-wasm-edge` is **excluded** from the workspace** (it targets `wasm32-unknown-unknown`, no_std). Build separately:
```bash
cargo build -p wifi-densepose-wasm-edge --target wasm32-unknown-unknown --release
# Standalone binary (e.g. ghost_hunter):
cargo build -p wifi-densepose-wasm-edge --bin ghost_hunter --target wasm32-unknown-unknown --release --no-default-features --features standalone-bin
```

### Python (legacy v1)
```bash
cd archive/v1
python -m pytest tests/ -x -q
```

### Deterministic proof (Trust Kill Switch)
```bash
python archive/v1/data/proof/verify.py
# Or the wrapper:
./verify
# With audit scan for mock/random patterns:
./verify --verbose --audit
# Regenerate expected hash if pipeline output changes:
python archive/v1/data/proof/verify.py --generate-hash
```

### Dashboard (nvsim-server UI)
```bash
cd dashboard
npm install
npm run build      # tsc --noEmit && vite build
npm run typecheck  # tsc --noEmit
npm test           # vitest run
```

### Firmware (ESP32-S3)
Built with ESP-IDF v5.4 in Docker (CI). Local build requires Python subprocess — see `CLAUDE.local.md` for the full command. Key: must strip MSYSTEM env vars for ESP-IDF on Git Bash.

## Key Gotchas

- **`--no-default-features` is required** for `cargo test` and `cargo check` on the workspace. The `eigenvalue` feature needs BLAS (ndarray-linalg with OpenBLAS) which may not be available. Without the flag, builds fail.
- **`wifi-densepose-wasm-edge` is excluded** from `cargo test --workspace` because it targets `wasm32-unknown-unknown` (no_std). Build it separately.
- **The proof pipeline is deterministic** — changing signal processing code in v1 will break the SHA-256 hash in `archive/v1/data/proof/expected_features.sha256`. If intentional, regenerate with `--generate-hash`.
- **Production code must not use `np.random.rand`/`np.random.randn`** — the `--audit` flag of `./verify` scans for these.
- **ESP32 builds fail on Git Bash** without stripping MSYSTEM env vars.
- **Publishing order matters** — crates must be published to crates.io in dependency order (see CLAUDE.md for the full 15-crate sequence).
- **The v1 path is `archive/v1/`**, not `v1/`. The Makefile target `test-rust` references `rust-port/wifi-densepose-rs` which is stale — use `v2/` instead.

## CI Pipeline

- **CI** (`.github/workflows/ci.yml`): code-quality (black, flake8, mypy, bandit) → rust-tests (`cargo test --workspace --no-default-features`) → Python tests (3.10/3.11/3.12 with postgres+redis services) → Docker build → docs deployment
- **Firmware CI**: ESP-IDF v5.4 Docker, builds 8MB + 4MB variants
- **`verify-pipeline.yml`**: Runs the deterministic proof check

## Architecture Notes

- **RuvSense modules** (`v2/crates/wifi-densepose-signal/src/ruvsense/`): 16 files implementing multistatic WiFi sensing — multiband fusion, phase alignment, coherence gating, pose tracking, RF tomography, etc.
- **Cross-viewpoint fusion** (`v2/crates/wifi-densepose-ruvector/src/viewpoint/`): 5 modules with attention-weighted fusion and geometric diversity.
- **Signal-Line CRV protocol** (`ruvector-crv`): 6-stage sensory→topology→coherence→search→model pipeline.
- **RuVector integration** (ADR-016, complete): 5 ruvector crates wired into signal, train, and ruvector workspace crates.
- **ADRs** govern all major decisions — 90+ in `docs/adr/`. Read the relevant ADR before making changes to any subsystem.

## Desktop App

`v2/crates/wifi-densepose-desktop/` is a Tauri app. Its `ui/` subdirectory has a separate `package.json` with Vite+React build.

## Docker

Two Dockerfiles in `docker/`:
- `Dockerfile.python` — legacy Python backend
- `Dockerfile.rust` — Rust sensing server

Docker Compose auto-detects ESP32 on UDP 5005, falls back to simulated data. Set `CSI_SOURCE=esp32` to force real hardware.