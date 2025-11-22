#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail

BUILD_DIR=build

cmake -S . -B $BUILD_DIR -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release
cmake --build $BUILD_DIR -j

exec ./$BUILD_DIR/mpz "$@"
