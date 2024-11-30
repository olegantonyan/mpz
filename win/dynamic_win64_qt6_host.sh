#!/bin/bash

source `dirname $0`/env_qt6_win64.sh

SRC_DIR=$(cd `dirname $0` && cd .. && pwd)
VERSION=$(grep -oP '(?<=").+(?=\\\\\\\")' $SRC_DIR/version.pri)
TMP_DIR=$(mktemp -d -t mpz-build-win64-qt6-$(date +%Y-%m-%d-%H-%M-%S)-XXXXX)
cd $TMP_DIR

ARTIFACT_NAME=mpz-$VERSION-win64-qt6-dynamic/

echo -e "version:\t$VERSION"
echo -e "source dir:\t$SRC_DIR"
echo -e "build dir:\t$TMP_DIR"

qmake.exe CONFIG+=release $SRC_DIR && mingw32-make.exe -j6
windeployqt6.exe ./app/mpz.exe --dir $ARTIFACT_NAME --compiler-runtime --release
cp ./app/mpz.exe $ARTIFACT_NAME
cp -R $QT_PATH/plugins/multimedia $ARTIFACT_NAME
rm -rf $HOME/Desktop/$ARTIFACT_NAME
cp -R $ARTIFACT_NAME $HOME/Desktop/

echo -e "version:\t$VERSION"
echo -e "source dir:\t$SRC_DIR"
echo -e "build dir:\t$TMP_DIR"

# gh release upload 1.0.19 ~/Desktop/mpz-1.0.19-win64-qt6-dynamic.zip
