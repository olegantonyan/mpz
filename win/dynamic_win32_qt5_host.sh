#!/bin/bash

export PATH=$PATH:/e/Qt/Tools/mingw810_32/bin/:/e/Qt/5.15.2/mingw81_32/bin/

SRC_DIR=$(cd `dirname $0` && cd .. && pwd)
VERSION=$(grep -oP '(?<=").+(?=\\\\\\\")' $SRC_DIR/version.pri)
TMP_DIR=$(mktemp -d -t mpz-build-win32-$(date +%Y-%m-%d-%H-%M-%S)-XXXXX)
cd $TMP_DIR

ARTIFACT_NAME=mpz-$VERSION-win32-qt5-dynamic/

echo -e "version:\t$VERSION"
echo -e "source dir:\t$SRC_DIR"
echo -e "build dir:\t$TMP_DIR"

qmake.exe CONFIG+=release $SRC_DIR && mingw32-make.exe -j6
windeployqt.exe ./app/mpz.exe --dir $ARTIFACT_NAME
cp ./app/mpz.exe $ARTIFACT_NAME
rm -rf $HOME/Desktop/$ARTIFACT_NAME
cp -R $ARTIFACT_NAME $HOME/Desktop/

echo -e "version:\t$VERSION"
echo -e "source dir:\t$SRC_DIR"
echo -e "build dir:\t$TMP_DIR"

# gh release upload 1.0.19 ~/Desktop/mpz-1.0.19-win32-dynamic.zip
