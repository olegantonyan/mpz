#!/bin/bash

##
QTMINGW_DIR=/mnt/storage/dev/qt/qt515-win64-mingw-static
MINGW_SYSROOT=/usr/x86_64-w64-mingw32/sys-root
MAKE=mingw64-make
##

SRC_DIR=$(cd `dirname $0` && pwd)
VERSION=$(grep -oP '(?<=").+(?=\\\\\\\")' $SRC_DIR/version.pri)
TMP_DIR=$(mktemp -d -t mpz-build-$(date +%Y-%m-%d-%H-%M-%S)-XXXXX)
TMP_SRC_DIR=mpz-$VERSION
TMP_SRC_FULL_DIR=$TMP_DIR/$TMP_SRC_DIR


echo -e "version:\t$VERSION"
echo -e "source dir:\t$SRC_DIR"
echo -e "tmp dir:\t$TMP_DIR"
echo -e "tmp source dir:\t$TMP_SRC_DIR"
echo -e "tmp source full dir:\t$TMP_SRC_FULL_DIR"


$QTMINGW_DIR/bin/qmake CONFIG+=release $SRC_DIR
$MAKE -j8

mkdir -p $TMP_SRC_FULL_DIR
cp -R app/mpz.exe                                   $TMP_SRC_FULL_DIR
cp -R $MINGW_SYSROOT/mingw/bin/libstdc++-6.dll      $TMP_SRC_FULL_DIR
cp -R $MINGW_SYSROOT/mingw/bin/libgcc_s_seh-1.dll   $TMP_SRC_FULL_DIR
cp -R $MINGW_SYSROOT/mingw/bin/libwinpthread-1.dll  $TMP_SRC_FULL_DIR
tree $TMP_SRC_FULL_DIR

cd $TMP_DIR
zip -r $TMP_SRC_DIR-win64.zip $TMP_SRC_DIR

echo "done"
echo $TMP_DIR/$TMP_SRC_DIR-win64.zip
mv $TMP_DIR/$TMP_SRC_DIR-win64.zip ~/Desktop
