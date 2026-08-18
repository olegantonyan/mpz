#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

source ./mac/_env.sh

PRESET="${1:-release-qt6}"

EXTRA_CMAKE_ARGS=("-DCMAKE_PREFIX_PATH=$QTDIR")

# Homebrew and MacPorts ship single-arch Qt, which cannot link the universal
# build the macOS presets ask for.
QT_CORE="$QTDIR/lib/QtCore.framework/Versions/A/QtCore"
QT_ARCHS=$(lipo -archs "$QT_CORE" 2>/dev/null || true)
if [[ "$QT_ARCHS" != *arm64* || "$QT_ARCHS" != *x86_64* ]]; then
  echo "Qt is not universal (${QT_ARCHS:-unknown}), building for $(uname -m) only"
  EXTRA_CMAKE_ARGS+=("-DCMAKE_OSX_ARCHITECTURES=$(uname -m)")
fi

BUILD_DIR="build/$PRESET"

cmake --preset "$PRESET" -B "$BUILD_DIR" "${EXTRA_CMAKE_ARGS[@]}"
cmake --build "$BUILD_DIR"

exec "$BUILD_DIR/mpz.app/Contents/MacOS/mpz" "${@:2}"
