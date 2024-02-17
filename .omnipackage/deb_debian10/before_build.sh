#!/usr/bin/env bash

set -xEeuo pipefail


GCC_VERSION="12.3.0"

if [ "$(gcc -dumpfullversion)" == $GCC_VERSION ]; then
    exit 0
fi

apt-get install -y curl build-essential libgmp-dev libmpfr-dev libmpc-dev

curl -s https://ftp.gnu.org/gnu/gcc/gcc-$GCC_VERSION/gcc-$GCC_VERSION.tar.xz | tar xJ
cd gcc-$GCC_VERSION/

./configure --enable-languages=c,c++ --disable-multilib
make -j$(nproc)
make install

cd ..
rm -rf gcc-$GCC_VERSION/
