#!/usr/bin/env bash
set -euo pipefail

PRESET="${1:-release-qt6}"

LAST_LINE=""
while IFS= read -r line; do
  echo "$line"
  LAST_LINE="$line"
done < <(cmake --preset "$PRESET" 2>&1)

if [[ "$LAST_LINE" =~ :\ (.*) ]]; then
  BUILD_DIR="${BASH_REMATCH[1]}"
else
  echo "Failed to extract build directory from last line."
  exit 1
fi
if [ ! -d "$BUILD_DIR" ]; then
  echo "Detected path is not a directory: $BUILD_DIR"
  exit 1
fi

cmake --build "$BUILD_DIR"

exec $BUILD_DIR/mpz
