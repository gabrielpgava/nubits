#!/usr/bin/env bash
set -euo pipefail

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"

QMAKE="/opt/homebrew/opt/qt@5/bin/qmake"
if [ ! -x "${QMAKE}" ]; then
    QMAKE="$(which qmake 2>/dev/null || true)"
fi

if [ -z "${QMAKE}" ] || [ ! -x "${QMAKE}" ]; then
    echo "Erro: qmake nao encontrado. Instale o Qt 5 via Homebrew: brew install qt@5"
    exit 1
fi

echo "==> Gerando Makefile com ${QMAKE}..."
"${QMAKE}" nu.pro

echo "==> Compilando Nu.app..."
make -j"$(sysctl -n hw.ncpu)" "$@"

echo "==> Nu.app compilado com sucesso em: ${DIR}/Nu.app"
