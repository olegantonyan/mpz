#!/bin/bash

QT_VERSION=5.15.2

QT_INSTALL_PATH=/e/Qt

QTDIR=$QT_INSTALL_PATH/$QT_VERSION/mingw81_32/
TOOLCHAIN_PATH=$QT_INSTALL_PATH/Tools/mingw810_32/bin
CMAKE_PATH=$QT_INSTALL_PATH/Tools/CMake_64/bin
NINJA_PATH=$QT_INSTALL_PATH/Tools/Ninja

export PATH=$PATH:$TOOLCHAIN_PATH:$QTDIR/bin/:$CMAKE_PATH:$NINJA_PATH
export QTDIR

echo -e "Qt path:\t$QTDIR"
echo -e "Toolchain:\t$TOOLCHAIN_PATH"
echo -e "CMake:\t$CMAKE_PATH"
echo -e "Ninja:\t$NINJA_PATH"

SRC_DIR=$(cd `dirname $0` && cd .. && pwd)
VERSION=$(gawk 'match($0, /project\(mpz VERSION (.+) LANGUAGES/, m) { print m[1]; }' < CMakeLists.txt | tr -d '\n')
TMP_DIR=$(mktemp -d -t mpz-build-win32-$(date +%Y-%m-%d-%H-%M-%S)-XXXXX)
cd $TMP_DIR

ARTIFACT_NAME=mpz-$VERSION-win32-qt5-portable/

echo -e "version:\t$VERSION"
echo -e "source dir:\t$SRC_DIR"
echo -e "build dir:\t$TMP_DIR"

cmake -DCMAKE_BUILD_TYPE=Release -GNinja -DUSE_QT5=ON $SRC_DIR && ninja
windeployqt.exe ./mpz.exe --dir $ARTIFACT_NAME
cp ./mpz.exe $ARTIFACT_NAME
rm -rf $HOME/Desktop/$ARTIFACT_NAME
cp -R $ARTIFACT_NAME $HOME/Desktop/

echo -e "version:\t$VERSION"
echo -e "source dir:\t$SRC_DIR"
echo -e "build dir:\t$TMP_DIR"

# gh release upload 1.0.19 ~/Desktop/mpz-1.0.19-win32-portable.zip
