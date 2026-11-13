# AGENTS.md — NuBits Project Context and Operations Guide

## Project language

NuBits is an international project. Use English as the default language for
project documentation (including README and AGENTS.md), code comments, commit
messages, and new user-facing text. Preserve existing translations and
localization resources; this policy does not require removing other languages
from them. Respond to users in their preferred language when appropriate.

## 1. Mainnet Recovery Node (Legacy VM)

The user maintains an active VM running a legacy NuBits version with a historical blockchain database found online to help recover the network:

- **Node IP (VM)**: `188.68.55.35`
- **Default mainnet P2P port**: `7890`
- **Default mainnet RPC ports**: `14001` (NuShares `S`) / `14002` (NuBits `B`)
- **Purpose**: Provide a reference seed/peer with the historical blockchain for synchronization, P2P/block protocol compatibility testing, and NuBits network recovery.

### Connecting the local client (`nud`) to the recovery VM
To synchronize or test the modern local binary against the legacy VM node:
```bash
# Connect exclusively to the recovery node:
./nud -connect=188.68.55.35:7890 -dnsseed=0

# Or add the node as a preferred peer:
./nud -addnode=188.68.55.35:7890
```

---

## 2. Architecture and Current Repository Status

- **Completed scope**: CLI / daemon (`nud`), test suite (`test_nu`), and graphical wallet (`Nu.app` / Qt 5.15).
- **Modernized stack (macOS Apple Silicon/Intel & Linux/WSL)**:
  - **C++17** (`-std=c++17`) with `-O2` optimization by default (`-O3` with `RELEASE=1`).
  - **Qt 5.15** (`qt@5` via Homebrew / Linux) with APIs migrated from Qt4 to Qt5 (`QtWidgets`, `QtConcurrent`, `QStandardPaths`, `QUrlQuery`, `setSectionResizeMode`).
  - **Native OpenSSL 3.x** (`openssl@3` / `libssl-dev`), with `CBigNum` wrapping `BIGNUM*` (`BN_new`/`BN_clear_free`), dynamic `EVP_CIPHER_CTX`, and strict DER normalization in `ECDSA_SIG`.
  - **Boost 1.83+ / 1.85** and **Berkeley DB 5.3** (`berkeley-db@5` / `libdb5.3++-dev`).

---

## 3. Quick Development Commands

### Build the Daemon (`nud`)
- **On Linux / WSL**:
```bash
./build_linux.sh -j$(nproc)
```
- **On macOS**:
```bash
./build_mac.sh -j$(sysctl -n hw.ncpu)
```

### Build and Run the Unit Test Suite (`test_nu` — 83 tests)
- **On Linux / WSL**:
```bash
make -C src -f makefile.unix -j$(nproc) test_nu
./src/test_nu --report_level=short
```
- **On macOS**:
```bash
make -C src -f makefile.osx -j$(sysctl -n hw.ncpu) test_nu
./src/test_nu --report_level=short
```

### Run from Windows PowerShell / CMD
The `nud.bat` wrapper makes it easier to interact with the daemon directly from Windows:
```powershell
# Start the daemon:
wsl -d Ubuntu -- bash -c "/mnt/c/Users/gabri/Downloads/nubits-main/nubits-main/nud -daemon"

# Query node status:
.\nud.bat getinfo

# Query connected peers:
.\nud.bat getpeerinfo
```

### Local Standalone Mode (`-localnet` / `-regtest`)
To test block generation and transactions locally without relying on the external network:
```bash
./run_localnet.sh start
./run_localnet.sh mine
./run_localnet.sh status
./run_localnet.sh stop
```
