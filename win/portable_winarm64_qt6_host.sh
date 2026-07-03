#!/bin/bash

source `dirname $0`/_env.sh

SRC_DIR=$(cd `dirname $0` && cd .. && pwd)
VERSION=$(`dirname $0`/_extract_version.sh)
OUTPUT_DIR=${MPZ_OUTPUT_DIR:-$HOME/Desktop}
TMP_DIR=$(mktemp -d -t mpz-build-winarm64-qt6-$(date +%Y-%m-%d-%H-%M-%S)-XXXXX)
cd $TMP_DIR

SUFFIX="${PACKAGE_VERSION:+-$PACKAGE_VERSION}"
ARTIFACT_NAME=mpz-$VERSION$SUFFIX-win-arm64-qt6-portable

echo -e "version:\t$VERSION"
echo -e "source dir:\t$SRC_DIR"
echo -e "build dir:\t$TMP_DIR"
echo -e "output dir:\t$OUTPUT_DIR"

EXTRA_CMAKE_ARGS=""
if [ -n "${PACKAGE_VERSION:-}" ]; then
    EXTRA_CMAKE_ARGS="-DPACKAGE_VERSION=$PACKAGE_VERSION"
fi

# Built with the MSVC ARM64 toolchain (set up before this script runs, e.g. via
# vcvarsall arm64). WINDEPLOYQT_EXTRA_ARGS is a hook for the cross-compiled Qt
# recipe (e.g. --qtpaths <arm64>/bin/qtpaths.bat); empty for the native
# windows_arm64 Qt where windeployqt6.exe resolves paths on its own.
cmake -DCMAKE_BUILD_TYPE=Release -GNinja $EXTRA_CMAKE_ARGS $SRC_DIR && ninja
windeployqt6.exe ./mpz.exe --dir $ARTIFACT_NAME --compiler-runtime --release ${WINDEPLOYQT_EXTRA_ARGS:-}
cp ./mpz.exe $ARTIFACT_NAME
cp -R $QTDIR/plugins/multimedia $ARTIFACT_NAME
copy_vc_runtime "$ARTIFACT_NAME"
rm -rf "$OUTPUT_DIR/$ARTIFACT_NAME" "$OUTPUT_DIR/$ARTIFACT_NAME.zip"
cp -R "$ARTIFACT_NAME" "$OUTPUT_DIR/"
(cd "$OUTPUT_DIR" && 7z a "$ARTIFACT_NAME.zip" "$ARTIFACT_NAME")

echo -e "version:\t$VERSION"
echo -e "source dir:\t$SRC_DIR"
echo -e "build dir:\t$TMP_DIR"
