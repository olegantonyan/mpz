#!/bin/bash

COMPILER="clang"
LIBAV_RELATIVE_PATH="ffmpeg-4.2.2"
CURRENT_DIRECTORY=`pwd`
BUILD_DIRECTORY=$CURRENT_DIRECTORY/ffmpeg_build
rm -rf $BUILD_DIRECTORY
mkdir -p $BUILD_DIRECTORY
SOURCE_DIRECTORY="$(cd `dirname $0` && pwd)/$LIBAV_RELATIVE_PATH"
echo "source directory: $SOURCE_DIRECTORY"
echo "build directory: $BUILD_DIRECTORY"
INSTALL_DIRECTORY="$CURRENT_DIRECTORY/libs/ffmpeg/"
echo "install prefix: $INSTALL_DIRECTORY"
rm -rf $INSTALL_DIRECTORY

cd $BUILD_DIRECTORY

$SOURCE_DIRECTORY/configure --disable-asm --disable-shared --disable-programs --disable-doc --disable-debug --cc=$COMPILER --prefix=$INSTALL_DIRECTORY --enable-libpulse && \
make -j8 && \
make install
