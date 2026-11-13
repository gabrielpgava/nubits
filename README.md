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
The graphical wallet additionally requires Qt 5.15. Install the development
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

Both scripts copy the built daemon to `./nud`. See
[`doc/`](doc/) for additional documentation and platform-specific build notes;
some historical instructions describe the legacy toolchain.

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

## Contributing

Keep documentation, commit messages, and new user-facing text in English so
contributors worldwide can collaborate. Preserve existing translations and
localization resources. Project-specific guidance for coding agents is in
[`AGENTS.md`](AGENTS.md).

## License

See [`COPYING`](COPYING) for the license terms.
