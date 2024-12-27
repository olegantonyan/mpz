#!/bin/bash

cd `dirname $0` && cd ..

version=$(gawk 'match($0, /project\(mpz VERSION (.+) LANGUAGES/, m) { print m[1]; }' < CMakeLists.txt | tr -d '\n')

printf '%s' "$version"

