#!/bin/bash

cd `dirname $0` && cd ..

version=$(grep -E 'project\(mpz VERSION' CMakeLists.txt | sed -E 's/.*VERSION ([0-9.]+).*/\1/' | tr -d '\n')

printf '%s' "$version"

