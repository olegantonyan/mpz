#!/bin/bash

# QT build:
# build deps: mingw64-cross-gcc-c++ mingw64-libstdc++6 mingw64-libwinpthread1 mingw64-libgcc_s_seh1
# https://download.qt.io/archive/qt/
# ./configure -nomake examples -nomake tests -nomake tools -confirm-license -xplatform win32-g++ -device-option CROSS_COMPILE=x86_64-w64-mingw32- -opensource -no-compile-examples -prefix /mnt/storage/dev/qt/qt515-win64-mingw-static -skip qt3d -skip qtandroidextras -skip qtcanvas3d -skip qtcharts -skip qtconnectivity -skip qtdatavis3d -skip qtdeclarative -skip qtdoc -skip qtgamepad -skip qtgraphicaleffects -skip qtimageformats -skip qtlocation -skip qtmacextras -skip qtquickcontrols -skip qtquickcontrols2 -skip qtscript -skip qtscxml -skip qtsensors -skip qtserialbus -skip qtserialport -skip qtspeech -skip activeqt -no-opengl -static -no-pch -optimize-size && make -j8 && make install

SRC_DIR=$(cd `dirname $0` && cd .. && pwd)
VERSION=$(grep -oP '(?<=").+(?=\\\\\\\")' $SRC_DIR/version.pri)
TMP_DIR=$(mktemp -d -t mpz-build-win64-$(date +%Y-%m-%d-%H-%M-%S)-XXXXX)
cd $TMP_DIR

##
QTMINGW_DIR=/mnt/storage/dev/qt/qt515-win64-mingw-static
MINGW_SYSROOT=/usr/x86_64-w64-mingw32/sys-root
MAKE=mingw64-make
ARTIFACT_NAME=mpz-$VERSION-win64-static.exe
##


echo -e "version:\t$VERSION"
echo -e "source dir:\t$SRC_DIR"
echo -e "build dir:\t$TMP_DIR"


$QTMINGW_DIR/bin/qmake CONFIG+=release CONFIG+=static QMAKE_LFLAGS+=-static DEFINES+=DISABLE_HTTPS $SRC_DIR && \
$MAKE -j8 && \
mv app/mpz.exe ~/Desktop/$ARTIFACT_NAME
