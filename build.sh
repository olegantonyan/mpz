#!/bin/bash

SRC_DIR=$(cd `dirname $0` && pwd)

$SRC_DIR/libs/libav/build.sh && \
$SRC_DIR/libs/libsoundio/build.sh && \
qmake-qt5 -spec linux-clang CONFIG+=release $SRC_DIR && \
make -j8
