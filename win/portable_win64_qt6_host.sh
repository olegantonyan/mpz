#!/bin/bash
set -e

source `dirname $0`/_env.sh

SRC_DIR=$(cd `dirname $0` && cd .. && pwd)
VERSION=$(`dirname $0`/_extract_version.sh)
OUTPUT_DIR=${MPZ_OUTPUT_DIR:-$HOME/Desktop}
TMP_DIR=$(mktemp -d -t mpz-build-win64-qt6-$(date +%Y-%m-%d-%H-%M-%S)-XXXXX)
cd $TMP_DIR

SUFFIX="${PACKAGE_VERSION:+-$PACKAGE_VERSION}"
ARTIFACT_NAME=mpz-$VERSION$SUFFIX-win-x86_64-qt6-portable

echo -e "version:\t$VERSION"
echo -e "source dir:\t$SRC_DIR"
echo -e "build dir:\t$TMP_DIR"
echo -e "output dir:\t$OUTPUT_DIR"

EXTRA_CMAKE_ARGS=""
if [ -n "${PACKAGE_VERSION:-}" ]; then
    EXTRA_CMAKE_ARGS="-DPACKAGE_VERSION=$PACKAGE_VERSION"
fi

cmake -DCMAKE_BUILD_TYPE=Release -GNinja $EXTRA_CMAKE_ARGS $SRC_DIR && ninja
# set -e misses a cmake-configure failure hidden inside `cmake && ninja`.
test -f ./mpz.exe || { echo "ERROR: build failed, mpz.exe was not produced" >&2; exit 1; }
# --no-compiler-runtime: windeployqt otherwise drops vc_redist.<arch>.exe (the
# redist *installer*) into the package in release mode. We ship the CRT DLLs
# app-local via copy_vc_runtime below instead, so suppress the installer.
windeployqt6.exe ./mpz.exe --dir $ARTIFACT_NAME --no-compiler-runtime --release
cp ./mpz.exe $ARTIFACT_NAME
# Ship the PDB so the crash handler can symbolize crash.log via dbghelp at runtime.
cp ./mpz.pdb $ARTIFACT_NAME
cp -R $QTDIR/plugins/multimedia $ARTIFACT_NAME
copy_vc_runtime "$ARTIFACT_NAME" x64
rm -rf "$OUTPUT_DIR/$ARTIFACT_NAME" "$OUTPUT_DIR/$ARTIFACT_NAME.zip"
cp -R "$ARTIFACT_NAME" "$OUTPUT_DIR/"
(cd "$OUTPUT_DIR" && 7z a "$ARTIFACT_NAME.zip" "$ARTIFACT_NAME")

echo -e "version:\t$VERSION"
echo -e "source dir:\t$SRC_DIR"
echo -e "build dir:\t$TMP_DIR"

# gh release upload 1.0.19 ~/Desktop/mpz-1.0.19-win64-qt6-portable.zip
