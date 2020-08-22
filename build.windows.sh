#!/bin/bash

SRC_DIR=$(cd `dirname $0` && pwd)
QTMINGW_DIR=/mnt/storage/dev/qt/qt515-win64-mingw-static

$QTMINGW_DIR/bin/qmake CONFIG+=release $SRC_DIR && mingw64-make -j8
