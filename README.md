# Nu Network Reference Wallet

Reference wallet and node software for the Nu network, supporting NuBits and
NuShares. This repository maintains the daemon, command-line RPC client, Qt
wallet, and unit tests, with a modernized build stack for macOS and Linux/WSL.

NuBits is an international project. English is the default language for project
documentation and contributions.

## Downloads

Prebuilt Windows and Linux headless packages, the Windows GUI installer, and
`SHA256SUMS` are published on the
[GitHub Releases page](https://github.com/gabrielpgava/nubits/releases).

For wallet information, see <https://nubits.com/wallet>. Exchanges integrating
NuBits or NuShares should also read <https://nubits.com/list-nubits>.

### Running a server release

Extract a server package into a directory writable by the account running NuBits,
then start `nud -daemon` on Linux or `nud.exe` on Windows.
The Linux x64 archive includes its non-glibc shared-library dependencies and
requires glibc 2.35 or newer (Ubuntu 22.04+). Keep the bundled `lib/` directory
beside `nud`.

NuBits checks GitHub Releases for newer versions. Automatic installation is
opt-in for unattended nodes with `-autoupdate=1`. Windows x64 and Linux x64
runtime packages are selected automatically and verified against their
published SHA-256 digest before installation. The repository and release
assets must be publicly accessible for this check to work.

## Building from source

The modernized build uses C++17, OpenSSL 3.x, Boost, and Berkeley DB 5.3.
The graphical wallet additionally requires Qt 6.12 or newer. Install the development
dependencies for your platform before building.

Run the following commands from the repository root.

### Linux / WSL

```bash
./build_linux.sh -j$(nproc)
```

### macOS

```bash
./build_mac.sh -j$(sysctl -n hw.ncpu)
```

For the graphical wallet, install the Qt 6 Homebrew modules without replacing
an existing Qt 5 installation:

```bash
brew install --skip-link qtbase qtsvg qtdeclarative qttools qttranslations
bash ./build_gui_mac.sh
open ./Nu.app
```

Qt 6.12 has a minimum macOS version of 14.4; installed Homebrew dependencies
may require a newer macOS version. Generated files live in `build/qt6/`,
separate from older Qt builds. This is a local build that uses Homebrew
libraries, not a portable or notarized distribution.

The daemon build scripts copy the built daemon to `./nud`. See
[`doc/`](doc/) for additional documentation and platform-specific build notes;
some historical instructions describe the legacy toolchain.

## Staking with NuShares

1. Select **NuShares (NSR)** and open the overview.
2. If the wallet is locked, choose **Unlock for staking** and enter its passphrase.
   This uses the existing minting-only authorization; sending funds still
   requires the normal wallet authorization.
3. Keep Nu running, connected, and synchronized. The overview shows wallet and
   network states separately; neither confirms that eligible coins are staking.

**Lock wallet** removes the minting-only authorization. NuBits (NBT) parking is
separate from NuShares staking. See the
[Qt wallet guide](doc/readme-qt.rst) for startup settings and build/test details.

## Unit tests

### Linux / WSL

```bash
make -C src -f makefile.unix -j$(nproc) test_nu
./src/test_nu --report_level=short
```

### macOS

```bash
make -C src -f makefile.osx -j$(sysctl -n hw.ncpu) test_nu
./src/test_nu --report_level=short
```

## Local standalone network

To test block generation and transactions without connecting to the public
network, build `nud` and use the local network helper:

```bash
./run_localnet.sh start
./run_localnet.sh mine
./run_localnet.sh status
./run_localnet.sh stop
```

The helper keeps its data in `.localnet-data/` by default. Override this location
with `NUBITS_LOCALNET_DIR` if needed. This environment is for local testing only;
do not use its predefined RPC credentials for a public node.

## Community

Join the [NuBits community forum](https://discuss.nubits.com/) to ask questions,
discuss the project, and connect with other community members.

### Network recovery discussion

The [network recovery and monetary policy proposal](doc/network-recovery-discussion.md)
records a possible direction for restoring block production, rewarding staking
and mining, and introducing fee burns and NSR-to-NBT conversion. It is an
exploratory proposal for community evaluation and criticism, not an approved
specification or implemented consensus change. No fiat peg is promised.

## Contributing

Keep documentation, commit messages, and new user-facing text in English so
contributors worldwide can collaborate. Preserve existing translations and
localization resources. Project-specific guidance for coding agents is in
[`AGENTS.md`](AGENTS.md).

## License

See [`COPYING`](COPYING) for the license terms.
