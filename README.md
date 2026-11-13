# Nu Network Reference Wallet

## NuBits and NuShares for Exchanges

Compile NuBits Wallet and read <https://nubits.com/list-nubits>.

## NuBits and NuShares for Individuals

Download NuBits Wallet for macOS, Windows, and Linux at <https://nubits.com/wallet>.

## Server releases

Prebuilt Windows and Linux headless packages, the Windows GUI installer, and
`SHA256SUMS` are published on the [GitHub Releases page](https://github.com/gabrielpgava/nubits/releases).
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
