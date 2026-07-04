#!/bin/bash

source `dirname $0`/_env.sh

SRC_DIR=$(cd `dirname $0` && cd .. && pwd)
VERSION=$(`dirname $0`/_extract_version.sh)
OUTPUT_DIR=${MPZ_OUTPUT_DIR:-$HOME/Desktop}
TMP_DIR=$(mktemp -d -t mpz-build-winarm64-qt6-$(date +%Y-%m-%d-%H-%M-%S)-XXXXX)
cd $TMP_DIR

cp -R $SRC_DIR/win/installer .
mkdir installer/packages/mpz/data

ARTIFACT_PATH=installer/packages/mpz/data/
SUFFIX="${PACKAGE_VERSION:+-$PACKAGE_VERSION}"
ARTIFACT_NAME=mpz-$VERSION$SUFFIX-win-arm64-qt6-installer.exe

echo -e "version:\t$VERSION"
echo -e "source dir:\t$SRC_DIR"
echo -e "build dir:\t$TMP_DIR"

EXTRA_CMAKE_ARGS=""
if [ -n "${PACKAGE_VERSION:-}" ]; then
    EXTRA_CMAKE_ARGS="-DPACKAGE_VERSION=$PACKAGE_VERSION"
fi

# See portable_winarm64_qt6_host.sh for notes on the MSVC/windeployqt setup.
cmake -DCMAKE_BUILD_TYPE=Release -GNinja $EXTRA_CMAKE_ARGS $SRC_DIR && ninja
windeployqt6.exe ./mpz.exe --dir $ARTIFACT_PATH --no-compiler-runtime --release ${WINDEPLOYQT_EXTRA_ARGS:-}
cp ./mpz.exe $ARTIFACT_PATH
cp -R $QTDIR/plugins/multimedia $ARTIFACT_PATH
copy_vc_runtime "$ARTIFACT_PATH" arm64

cd installer
cp $SRC_DIR/license.txt packages/mpz/meta/

sed -Ei "s/<Version>(.*)<\/Version>/<Version>$VERSION<\/Version>/g" ./config/config.xml
sed -Ei "s/<Version>(.*)<\/Version>/<Version>$VERSION<\/Version>/g" ./packages/mpz/meta/package.xml
sed -Ei "s/<ReleaseDate>(.*)<\/ReleaseDate>/<ReleaseDate>$(date +%Y-%m-%d)<\/ReleaseDate>/g" ./packages/mpz/meta/package.xml

cat ./config/config.xml
cat ./packages/mpz/meta/package.xml

binarycreator.exe --offline-only -c config/config.xml -p packages $ARTIFACT_NAME
cp "./$ARTIFACT_NAME" "$OUTPUT_DIR/"
