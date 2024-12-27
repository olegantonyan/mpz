#!/bin/bash

source `dirname $0`/_env.sh

SRC_DIR=$(cd `dirname $0` && cd .. && pwd)
VERSION=$(`dirname $0`/extract_version.sh)
TMP_DIR=$(mktemp -d -t mpz-build-win64-qt6-$(date +%Y-%m-%d-%H-%M-%S)-XXXXX)
cd $TMP_DIR

cp -R $SRC_DIR/win/installer .
mkdir installer/packages/mpz/data

ARTIFACT_PATH=installer/packages/mpz/data/
ARTIFACT_NAME=mpz-$VERSION-win64-qt6-installer.exe

echo -e "version:\t$VERSION"
echo -e "source dir:\t$SRC_DIR"
echo -e "build dir:\t$TMP_DIR"

cmake -DCMAKE_BUILD_TYPE=Release -GNinja $SRC_DIR && ninja
windeployqt6.exe ./mpz.exe --dir $ARTIFACT_PATH --compiler-runtime --release
cp ./mpz.exe $ARTIFACT_PATH
cp -R $QTDIR/plugins/multimedia $ARTIFACT_PATH

cd installer
cp $SRC_DIR/license.txt packages/mpz/meta/

sed -Ei "s/<Version>(.*)<\/Version>/<Version>$VERSION<\/Version>/g" ./config/config.xml
sed -Ei "s/<Version>(.*)<\/Version>/<Version>$VERSION<\/Version>/g" ./packages/mpz/meta/package.xml
sed -Ei "s/<ReleaseDate>(.*)<\/ReleaseDate>/<ReleaseDate>$(date +%Y-%m-%d)<\/ReleaseDate>/g" ./packages/mpz/meta/package.xml

cat ./config/config.xml
cat ./packages/mpz/meta/package.xml

binarycreator.exe --offline-only -c config/config.xml -p packages $ARTIFACT_NAME
cp ./$ARTIFACT_NAME $HOME/Desktop


