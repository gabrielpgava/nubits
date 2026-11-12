#!/bin/bash
# NuBits Daemon Auto-Updater for Linux / POSIX

ASSET_PATH="$1"
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "======================================================="
echo "        NuBits Daemon Auto-Updater for Linux"
echo "======================================================="

echo "[1/4] Aguardando o encerramento do daemon..."
sleep 3

killall -9 nud 2>/dev/null || true

echo "[2/4] Verificando pacote de atualizacao: $ASSET_PATH"
if [ ! -f "$ASSET_PATH" ]; then
    echo "ERRO: Arquivo de atualizacao nao encontrado: $ASSET_PATH"
    exit 1
fi

echo "[3/4] Aplicando nova versao..."
if [ -f "$ROOT_DIR/nud" ]; then
    cp "$ROOT_DIR/nud" "$ROOT_DIR/nud.bak"
fi

if [[ "$ASSET_PATH" == *.tar.gz ]]; then
    tar -xzf "$ASSET_PATH" -C "$ROOT_DIR"
elif [[ "$ASSET_PATH" == *.zip ]]; then
    unzip -o "$ASSET_PATH" -d "$ROOT_DIR"
else
    cp -f "$ASSET_PATH" "$ROOT_DIR/nud"
fi

chmod +x "$ROOT_DIR/nud"

echo "[4/4] Reiniciando daemon nud..."
nohup "$ROOT_DIR/nud" -daemon >/dev/null 2>&1 &

echo "Atualizacao concluida com sucesso!"
exit 0
