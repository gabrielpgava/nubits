#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NUD="${ROOT_DIR}/nud"
if [ ! -x "${NUD}" ]; then
    NUD="${ROOT_DIR}/src/nud"
fi

DATADIR="${NUBITS_LOCALNET_DIR:-${ROOT_DIR}/.localnet-data}"
mkdir -p "${DATADIR}"

CONF="${DATADIR}/nu.conf"
if [ ! -f "${CONF}" ]; then
    cat > "${CONF}" <<EOF
rpcuser=nubitsrpc
rpcpassword=localnetpass
server=1
listen=0
localnet=1
keypool=10
EOF
    chmod 600 "${CONF}"
fi

CMD="${1:-help}"
shift || true

case "${CMD}" in
    start)
        echo "==> Iniciando nud em modo standalone (-localnet) em ${DATADIR}..."
        "${NUD}" -datadir="${DATADIR}" -localnet -daemon "$@"
        printf "==> Aguardando RPC ficar pronto"
        for _ in {1..30}; do
            if "${NUD}" -datadir="${DATADIR}" -localnet getblockcount >/dev/null 2>&1; then
                echo " OK!"
                exit 0
            fi
            printf "."
            sleep 0.5
        done
        echo " Timeout aguardando RPC!"
        exit 1
        ;;
    mine)
        echo "==> Ativando mineracao local de blocos (setgenerate true)..."
        "${NUD}" -datadir="${DATADIR}" -localnet setgenerate true "$@"
        ;;
    stop)
        echo "==> Parando nud (-localnet)..."
        "${NUD}" -datadir="${DATADIR}" -localnet stop
        ;;
    status|info)
        "${NUD}" -datadir="${DATADIR}" -localnet getinfo
        ;;
    rpc)
        "${NUD}" -datadir="${DATADIR}" -localnet "$@"
        ;;
    *)
        echo "Uso: ./run_localnet.sh {start|mine|status|stop|rpc <comando> [args...]}"
        echo ""
        echo "Exemplos:"
        echo "  ./run_localnet.sh start"
        echo "  ./run_localnet.sh mine"
        echo "  ./run_localnet.sh status"
        echo "  ./run_localnet.sh rpc getnewaddress"
        echo "  ./run_localnet.sh stop"
        ;;
esac
