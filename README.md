# Nu Network Reference Wallet

## NuBits and NuShares for Exchanges

Compile NuBits Wallet and read <https://nubits.com/list-nubits>.

## NuBits and NuShares for Individuals

Download NuBits Wallet for macOS, Windows, and Linux at <https://nubits.com/wallet>.

## Server releases

Prebuilt Windows and Linux headless packages, the Windows GUI installer, and
`SHA256SUMS` are published on the [GitHub Releases page](https://github.com/gabrielpgava/nubits/releases).
Extract a server package into a directory writable by the account running NuBits,
then start `nud -daemon` on Linux or `nud.exe -daemon` on Windows.

NuBits checks GitHub Releases for newer versions. Automatic installation is
opt-in for unattended nodes with `-autoupdate=1`; update packages are selected
for the current OS/architecture and verified against their published SHA-256
digest before installation. The repository and release assets must be publicly
accessible for this check to work.
