#!/bin/bash

QT_PATH=/e/Qt/6.7.1/mingw_64/
TOOLCHAIN_PATH=/e/Qt/Tools/mingw1120_64/bin
INSTALLER_FRAMEWORK_PATH=/e/Qt/Tools/QtInstallerFramework/4.8/bin

export PATH=$PATH:$TOOLCHAIN_PATH:$QT_PATH/bin/:$INSTALLER_FRAMEWORK_PATH

SRC_DIR=$(cd `dirname $0` && cd .. && pwd)
VERSION=$(grep -oP '(?<=").+(?=\\\\\\\")' $SRC_DIR/version.pri)
TMP_DIR=$(mktemp -d -t mpz-build-win64-qt6-$(date +%Y-%m-%d-%H-%M-%S)-XXXXX)
cd $TMP_DIR

cp -R $SRC_DIR/win/installer .
mkdir installer/packages/mpz/data

ARTIFACT_PATH=installer/packages/mpz/data/
ARTIFACT_NAME=mpz-$VERSION-win64-qt6-installer.exe

echo -e "version:\t$VERSION"
echo -e "source dir:\t$SRC_DIR"
echo -e "build dir:\t$TMP_DIR"

qmake.exe CONFIG+=release $SRC_DIR && mingw32-make.exe -j6
windeployqt.exe ./app/mpz.exe --dir $ARTIFACT_PATH
cp ./app/mpz.exe $ARTIFACT_PATH
cp -R $QT_PATH/plugins/multimedia $ARTIFACT_PATH

cd installer
cp $SRC_DIR/license.txt packages/mpz/meta/

sed -Ei "s/<Version>(.*)<\/Version>/<Version>$VERSION<\/Version>/g" ./config/config.xml
sed -Ei "s/<Version>(.*)<\/Version>/<Version>$VERSION<\/Version>/g" ./packages/mpz/meta/package.xml
sed -Ei "s/<ReleaseDate>(.*)<\/ReleaseDate>/<ReleaseDate>$(date +%Y-%m-%d)<\/ReleaseDate>/g" ./packages/mpz/meta/package.xml

cat ./config/config.xml
cat ./packages/mpz/meta/package.xml

binarycreator.exe -c config/config.xml -p packages $ARTIFACT_NAME
cp ./$ARTIFACT_NAME $HOME/Desktop


