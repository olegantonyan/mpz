#!/bin/bash
set -e

if [ -z "${QTDIR:-}" ]; then
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
fi

SRC_DIR=$(cd `dirname $0` && cd .. && pwd)
VERSION=$(`dirname $0`/_extract_version.sh)
OUTPUT_DIR=${MPZ_OUTPUT_DIR:-$HOME/Desktop}
TMP_DIR=$(mktemp -d -t mpz-build-win32-$(date +%Y-%m-%d-%H-%M-%S)-XXXXX)
cd $TMP_DIR

SUFFIX="${PACKAGE_VERSION:+-$PACKAGE_VERSION}"
ARTIFACT_NAME=mpz-$VERSION$SUFFIX-win-legacy-qt5-portable

echo -e "version:\t$VERSION"
echo -e "source dir:\t$SRC_DIR"
echo -e "build dir:\t$TMP_DIR"
echo -e "output dir:\t$OUTPUT_DIR"

EXTRA_CMAKE_ARGS=""
if [ -n "${PACKAGE_VERSION:-}" ]; then
    EXTRA_CMAKE_ARGS="-DPACKAGE_VERSION=$PACKAGE_VERSION"
fi

cmake -DCMAKE_BUILD_TYPE=Release -GNinja -DUSE_QT5=ON $EXTRA_CMAKE_ARGS $SRC_DIR && ninja
# set -e misses a cmake-configure failure hidden inside `cmake && ninja`.
test -f ./mpz.exe || { echo "ERROR: build failed, mpz.exe was not produced" >&2; exit 1; }
windeployqt.exe ./mpz.exe --dir $ARTIFACT_NAME
cp ./mpz.exe $ARTIFACT_NAME
rm -rf "$OUTPUT_DIR/$ARTIFACT_NAME" "$OUTPUT_DIR/$ARTIFACT_NAME.zip"
cp -R "$ARTIFACT_NAME" "$OUTPUT_DIR/"
(cd "$OUTPUT_DIR" && 7z a "$ARTIFACT_NAME.zip" "$ARTIFACT_NAME")

echo -e "version:\t$VERSION"
echo -e "source dir:\t$SRC_DIR"
echo -e "build dir:\t$TMP_DIR"

# gh release upload 1.0.19 ~/Desktop/mpz-1.0.19-win32-portable.zip
