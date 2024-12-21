#!/bin/bash

source `dirname $0`/env_qt6_win64.sh

SRC_DIR=$(cd `dirname $0` && cd .. && pwd)
VERSION=$(gawk 'match($0, /project\(mpz VERSION (.+) LANGUAGES/, m) { print m[1]; }' < CMakeLists.txt | tr -d '\n')
TMP_DIR=$(mktemp -d -t mpz-build-win64-qt6-$(date +%Y-%m-%d-%H-%M-%S)-XXXXX)
cd $TMP_DIR

ARTIFACT_NAME=mpz-$VERSION-win64-qt6-portable/

echo -e "version:\t$VERSION"
echo -e "source dir:\t$SRC_DIR"
echo -e "build dir:\t$TMP_DIR"

cmake -DCMAKE_BUILD_TYPE=Release -GNinja $SRC_DIR && ninja
windeployqt6.exe ./mpz.exe --dir $ARTIFACT_NAME --compiler-runtime --release
cp ./mpz.exe $ARTIFACT_NAME
cp -R $QTDIR/plugins/multimedia $ARTIFACT_NAME
rm -rf $HOME/Desktop/$ARTIFACT_NAME
cp -R $ARTIFACT_NAME $HOME/Desktop/

echo -e "version:\t$VERSION"
echo -e "source dir:\t$SRC_DIR"
echo -e "build dir:\t$TMP_DIR"

# gh release upload 1.0.19 ~/Desktop/mpz-1.0.19-win64-qt6-portable.zip
