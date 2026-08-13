#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

PRESET="${1:-release-qt6-syslibs}"

BUILD_DIR="build/$PRESET"

cmake --preset "$PRESET" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR"

exec "$BUILD_DIR/mpz" "${@:2}"
