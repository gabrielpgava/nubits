#!/usr/bin/env bash
set -euo pipefail

VERSION="${1:-}"
if [[ ! "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "Usage: $0 <major.minor.patch>" >&2
    exit 2
fi

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIST_DIR="$ROOT_DIR/dist"
PROJECT_VERSION="$(sed -n 's/^VERSION = //p' "$ROOT_DIR/nu.pro")"
if [[ "$VERSION" != "$PROJECT_VERSION" ]]; then
    echo "Tag version $VERSION does not match nu.pro version $PROJECT_VERSION" >&2
    exit 1
fi

for command in ldd patchelf tar; do
    if ! command -v "$command" >/dev/null 2>&1; then
        echo "Required packaging command not found: $command" >&2
        exit 1
    fi
done

if [[ "$(uname -m)" != "x86_64" ]]; then
    echo "This release package is for Linux x86_64, not $(uname -m)" >&2
    exit 1
fi

if ldd "$ROOT_DIR/src/nud" | grep -q 'not found'; then
    echo "The Linux daemon has unresolved shared-library dependencies" >&2
    ldd "$ROOT_DIR/src/nud" >&2
    exit 1
fi

mkdir -p "$DIST_DIR"
STAGE_DIR="$(mktemp -d "$DIST_DIR/.linux-server.XXXXXX")"
trap 'rm -rf "$STAGE_DIR"' EXIT
mkdir -p "$STAGE_DIR/lib"

cp "$ROOT_DIR/src/nud" "$ROOT_DIR/update_posix.sh" "$ROOT_DIR/COPYING" \
    "$ROOT_DIR/doc/README" "$STAGE_DIR/"
cp /etc/ssl/certs/ca-certificates.crt "$STAGE_DIR/cacert.pem"
chmod 0755 "$STAGE_DIR/nud" "$STAGE_DIR/update_posix.sh"

while read -r first second third _; do
    if [[ "$second" == "=>" && "$third" == /* ]]; then
        library="$third"
    elif [[ "$first" == /* ]]; then
        library="$first"
    else
        continue
    fi

    soname="$(basename "$library")"
    case "$soname" in
        libc.so.*|libm.so.*|libpthread.so.*|librt.so.*|libdl.so.*|libresolv.so.*|libutil.so.*|libnss_*.so.*|ld-linux*)
            continue
            ;;
    esac
    cp -L "$library" "$STAGE_DIR/lib/$soname"
done < <(ldd "$ROOT_DIR/src/nud")

patchelf --force-rpath --set-rpath '$ORIGIN/lib' "$STAGE_DIR/nud"
while IFS= read -r -d '' library; do
    patchelf --force-rpath --set-rpath '$ORIGIN' "$library"
done < <(find "$STAGE_DIR/lib" -type f -name '*.so*' -print0)

if ldd "$STAGE_DIR/nud" | grep -q 'not found'; then
    echo "Linux runtime package is missing one or more shared libraries" >&2
    ldd "$STAGE_DIR/nud" >&2
    exit 1
fi

SMOKE_DIR="$STAGE_DIR/.smoke"
mkdir -p "$SMOKE_DIR"
"$STAGE_DIR/nud" -datadir="$SMOKE_DIR" --help >/dev/null

PACKAGE="$DIST_DIR/Nu-$VERSION-linux-x64-server.tar.gz"
tar -C "$STAGE_DIR" -czf "$PACKAGE" nud update_posix.sh cacert.pem lib COPYING README
tar -tzf "$PACKAGE"
echo "Created $PACKAGE"
