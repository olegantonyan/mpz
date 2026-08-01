#!/bin/bash

cd "$(dirname "${BASH_SOURCE[0]}")/.." || exit 1

version=$(grep -E 'project\(mpz VERSION' CMakeLists.txt | sed -E 's/.*VERSION ([0-9.]+).*/\1/' | tr -d '\n')

printf '%s' "$version"

