#!/usr/bin/env bash
set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PREFIX="$DIR/openssl"
BUILD_DIR="/tmp/openssl-1.0.2u-build"

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "Downloading OpenSSL 1.0.2u..."
curl -L -O https://github.com/openssl/openssl/releases/download/OpenSSL_1_0_2u/openssl-1.0.2u.tar.gz
tar -xzf openssl-1.0.2u.tar.gz
cd openssl-1.0.2u

ARCH=$(uname -m)
if [ "$ARCH" = "arm64" ]; then
    TARGET="darwin64-arm64-cc"
    # Inject darwin64-arm64-cc target into Configure if missing
    if ! grep -q "darwin64-arm64-cc" Configure; then
        perl -pi -e 's/"darwin64-x86_64-cc",/"darwin64-arm64-cc","cc:-arch arm64 -O3 -DL_ENDIAN -Wall::-D_REENTRANT:MACOSX:-Wl,-search_paths_first%:SIXTY_FOUR_BIT_LONG RC4_CHUNK DES_INT DES_UNROLL:\${no_asm}:macosx:dlfcn:darwin-shared:-fPIC -fno-common:-arch arm64 -dynamiclib:\\\$(SHLIB_MAJOR).\\\$(SHLIB_MINOR).dylib",\n"darwin64-x86_64-cc",/' Configure
    fi
    ./Configure "$TARGET" no-asm no-shared
else
    ./Configure darwin64-x86_64-cc no-shared
fi

make -j$(sysctl -n hw.ncpu)

mkdir -p "$PREFIX/include" "$PREFIX/lib"
cp -RL include/openssl "$PREFIX/include/"
cp libcrypto.a libssl.a "$PREFIX/lib/"

echo "OpenSSL 1.0.2u built and installed to $PREFIX successfully."
