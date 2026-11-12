#!/usr/bin/env bash
set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR/src"

echo "Building nud for Linux..."
make -f makefile.unix "$@"

cp -f nud "$DIR/nud"
echo "Build complete: $DIR/nud"
