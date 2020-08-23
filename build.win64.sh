#!/bin/bash

##
QTMINGW_DIR=/mnt/storage/dev/qt/qt515-win64-mingw-static
MINGW_SYSROOT=/usr/x86_64-w64-mingw32/sys-root
MAKE=mingw64-make
##

SRC_DIR=$(cd `dirname $0` && pwd)
VERSION=$(grep -oP '(?<=").+(?=\\\\\\\")' $SRC_DIR/version.pri)


echo -e "version:\t$VERSION"
echo -e "source dir:\t$SRC_DIR"


$QTMINGW_DIR/bin/qmake CONFIG+=release CONFIG+=static QMAKE_LFLAGS+=-static $SRC_DIR && \
$MAKE -j8 && \
mv app/mpz.exe ~/Desktop/mpz-$VERSION-win64-static.exe
