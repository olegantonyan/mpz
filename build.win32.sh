#!/bin/bash

# QT build:
# build deps: mingw32-cross-gcc-c++ mingw32-libgcc_s_sjlj1 mingw32-libstdc++6
# https://download.qt.io/archive/qt/
# ./configure -nomake examples -nomake tests -nomake tools -confirm-license -xplatform win32-g++ -device-option CROSS_COMPILE=i686-w64-mingw32- -opensource -no-compile-examples -prefix /mnt/storage/dev/qt/qt515-win32-mingw-static -skip qt3d -skip qtandroidextras -skip qtcanvas3d -skip qtcharts -skip qtconnectivity -skip qtdatavis3d -skip qtdeclarative -skip qtdoc -skip qtgamepad -skip qtgraphicaleffects -skip qtimageformats -skip qtlocation -skip qtmacextras -skip qtquickcontrols -skip qtquickcontrols2 -skip qtscript -skip qtscxml -skip qtsensors -skip qtserialbus -skip qtserialport -skip qtspeech -skip activeqt -no-opengl -static -no-pch -optimize-size && make -j8 && make install

##
QTMINGW_DIR=/mnt/storage/dev/qt/qt515-win32-mingw-static
MINGW_SYSROOT=/usr/i686-w64-mingw32/sys-root/
MAKE=mingw32-make
ARTIFACT_NAME=mpz-$VERSION-win32-static.exe
##

SRC_DIR=$(cd `dirname $0` && pwd)
VERSION=$(grep -oP '(?<=").+(?=\\\\\\\")' $SRC_DIR/version.pri)


echo -e "version:\t$VERSION"
echo -e "source dir:\t$SRC_DIR"

$MAKE clean
rm Makefile
$QTMINGW_DIR/bin/qmake CONFIG+=release CONFIG+=static QMAKE_LFLAGS+=-static $SRC_DIR && \
$MAKE -j8 && \
mv app/mpz.exe ~/Desktop/$ARTIFACT_NAME
