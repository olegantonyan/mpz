#!/usr/bin/env bash
set -euo pipefail

PRESET="${1:-release-qt6}"

BUILD_DIR=""

while IFS= read -r line; do
  echo "$line"
  if [[ "$line" =~ --\ Build\ files\ have\ been\ written\ to:\ (.*) ]]; then
    BUILD_DIR="${BASH_REMATCH[1]}"
  fi
done < <(cmake --preset "$PRESET" 2>&1)

if [ -z "$BUILD_DIR" ]; then
  echo "Failed to detect build directory from cmake output."
  exit 1
fi

cmake --build "$BUILD_DIR"

exec $BUILD_DIR/mpz
