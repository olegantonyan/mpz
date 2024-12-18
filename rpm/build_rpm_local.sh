#!/bin/bash

SRC_DIR=$(cd `dirname $0` && cd .. && pwd)
VERSION=$(gawk 'match($0, /project\(mpz VERSION (.+) LANGUAGES/, m) { print m[1]; }' < CMakeLists.txt | tr -d '\n')
TMP_DIR=$(mktemp -d -t mpz-build-$(date +%Y-%m-%d-%H-%M-%S)-XXXXX)
TMP_SRC_DIR=mpz-$VERSION
TMP_SRC_FULL_DIR=$TMP_DIR/mpz-$VERSION
SPECFILE=rpm/mpz.spec

echo -e "version:\t$VERSION"
echo -e "source dir:\t$SRC_DIR"
echo -e "tmp dir:\t$TMP_DIR"
echo -e "tmp source dir:\t$TMP_SRC_DIR"
echo -e "tmp source full dir:\t$TMP_SRC_FULL_DIR"

mkdir -p   $TMP_SRC_FULL_DIR/
cp -ar ./* $TMP_SRC_FULL_DIR/
ls -latrh $TMP_SRC_FULL_DIR

tree $TMP_SRC_FULL_DIR

cd $TMP_DIR
SRC_ARTIFACT=$TMP_SRC_DIR.tar.gz
tar -cvzf $SRC_ARTIFACT $TMP_SRC_DIR
rpmdev-setuptree
mv $SRC_ARTIFACT ~/rpmbuild/SOURCES

cd $SRC_DIR
cp $SPECFILE ~/rpmbuild/SPECS
tree ~/rpmbuild/

# rpmbuild --with qt6 -bs $SPECFILE
rpmbuild --with qt6 -bb $SPECFILE


rm -rf $TMP_DIR
