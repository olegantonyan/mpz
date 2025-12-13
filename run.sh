#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail

PRESET=release-qt6
BUILD_DIR=build/$PRESET

cmake --preset $PRESET
cmake --build $BUILD_DIR

exec ./$BUILD_DIR/mpz "$@"
