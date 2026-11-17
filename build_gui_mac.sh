#!/usr/bin/env bash
set -euo pipefail

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"

QT_PREFIX="${QT_PREFIX:-/opt/homebrew/opt/qtbase}"
if [ ! -x "${QT_PREFIX}/bin/qmake" ] && [ -x /usr/local/opt/qtbase/bin/qmake ]; then
    QT_PREFIX=/usr/local/opt/qtbase
fi
QT_TOOLS_PREFIX="${QT_PREFIX%/qtbase}/qttools"
QT_TRANSLATIONS_PREFIX="${QT_PREFIX%/qtbase}/qttranslations"
QMAKE="${QT_PREFIX}/bin/qmake"
LRELEASE="${QT_TOOLS_PREFIX}/bin/lrelease"

if [ ! -x "$QMAKE" ] || [ ! -x "$LRELEASE" ]; then
    echo "Qt 6 tools not found. Install qtbase, qttools and qttranslations with Homebrew."
    exit 1
fi

BUILD_DIR="${BUILD_DIR:-${DIR}/build/qt6}"
DESTDIR="${DESTDIR:-${DIR}}"
mkdir -p "$BUILD_DIR" "$DESTDIR"
BUILD_DIR="$(cd "$BUILD_DIR" && pwd)"
DESTDIR="$(cd "$DESTDIR" && pwd)"
# Unlinked Homebrew Qt 6 must not pick up the existing Qt 5 tools or plugins.
printf '[Paths]\nPrefix=%s\nArchData=share/qt\nData=share/qt\nLibraryExecutables=share/qt/libexec\nHostData=share/qt\nHostLibraryExecutables=share/qt/libexec\nPlugins=share/qt/plugins\n' \
    "$QT_PREFIX" > "$BUILD_DIR/qt6.conf"

echo "==> Generating a Qt 6 Makefile..."
cd "$BUILD_DIR"
"$QMAKE" -qtconf "$BUILD_DIR/qt6.conf" "$DIR/nu.pro" \
    "QT_TOOL.lrelease.binary=$LRELEASE" "DESTDIR=$DESTDIR"

echo "==> Building Nu.app..."
make -j"${JOBS:-$(sysctl -n hw.ncpu)}" "$@"

APP="$DESTDIR/Nu.app"
mkdir -p "$APP/Contents/Resources"
printf '[Paths]\nPrefix=%s\nPlugins=share/qt/plugins\nTranslations=%s/share/qt/translations\n' \
    "$QT_PREFIX" "$QT_TRANSLATIONS_PREFIX" > "$APP/Contents/Resources/qt.conf"
codesign --force --sign - --timestamp=none "$APP"
echo "==> Local build ready: $APP"
