# AGENTS.md — Guia de Contexto e Operação do Projeto NuBits

## 1. Nó de Recuperação da Rede Mainnet (VM Legada)

Existe uma VM ativa mantida pelo usuário rodando uma versão legada do NuBits com o banco de dados histórico da blockchain (encontrado online) com o objetivo de recuperar a rede:

- **IP do Nó (VM)**: `188.68.55.35`
- **Porta P2P Mainnet padrão**: `7890`
- **Porta RPC Mainnet padrão**: `14001` (NuShares `S`) / `14002` (NuBits `B`)
- **Finalidade**: Servir como seed/peer de referência com a blockchain histórica para sincronização, testes de compatibilidade de protocolo P2P/blocos e recuperação da rede NuBits.

### Como conectar o cliente local (`nud`) à VM de recuperação
Para sincronizar ou testar a conexão do binário local moderno com o nó legado da VM:
```bash
# Conectando exclusivamente ao nó de recuperação:
./nud -connect=188.68.55.35:7890 -dnsseed=0

# Ou adicionando o nó como peer prioritário:
./nud -addnode=188.68.55.35:7890
```

---

## 2. Arquitetura e Estado Atual do Repositório

- **Foco Concluído**: CLI / Daemon (`nud`), suíte de testes (`test_nu`) e Carteira Gráfica (`Nu.app` / Qt 5.15).
- **Stack Modernizada (macOS Apple Silicon/Intel & Linux/WSL)**:
  - **C++17** (`-std=c++17`) com otimização `-O2` por padrão (`-O3` com `RELEASE=1`).
  - **Qt 5.15** (`qt@5` via Homebrew / Linux) com APIs migradas de Qt4 para Qt5 (`QtWidgets`, `QtConcurrent`, `QStandardPaths`, `QUrlQuery`, `setSectionResizeMode`).
  - **OpenSSL 3.x nativo** (`openssl@3` / `libssl-dev`), com `CBigNum` encapsulando `BIGNUM*` (`BN_new`/`BN_clear_free`), `EVP_CIPHER_CTX` dinâmico e normalização DER estrita em `ECDSA_SIG`.
  - **Boost 1.83+ / 1.85** e **Berkeley DB 5.3** (`berkeley-db@5` / `libdb5.3++-dev`).

---

## 3. Comandos Rápidos de Desenvolvimento

### Compilar o Daemon (`nud`)
- **No Linux / WSL**:
```bash
./build_linux.sh -j$(nproc)
```
- **No macOS**:
```bash
./build_mac.sh -j$(sysctl -n hw.ncpu)
```

### Compilar e Rodar a Suíte de Testes Unitários (`test_nu` — 83 testes)
- **No Linux / WSL**:
```bash
make -C src -f makefile.unix -j$(nproc) test_nu
./src/test_nu --report_level=short
```
- **No macOS**:
```bash
make -C src -f makefile.osx -j$(sysctl -n hw.ncpu) test_nu
./src/test_nu --report_level=short
```

### Executar a partir do Windows PowerShell / CMD
Foi criado o wrapper `nud.bat` para facilitar a interação com o daemon direto do Windows:
```powershell
# Iniciar daemon:
wsl -d Ubuntu -- bash -c "/mnt/c/Users/gabri/Downloads/nubits-main/nubits-main/nud -daemon"

# Consultar status do nó:
.\nud.bat getinfo

# Consultar peers conectados:
.\nud.bat getpeerinfo
```

### Modo Standalone Local (`-localnet` / `-regtest`)
Para testar mineração de blocos e transações localmente sem depender da rede externa:
```bash
./run_localnet.sh start
./run_localnet.sh mine
./run_localnet.sh status
./run_localnet.sh stop
```
