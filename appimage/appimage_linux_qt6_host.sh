#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
SRC_DIR=$(cd "$SCRIPT_DIR/.." && pwd)

# Optional arg: AppImage runtime file. Without it appimagetool downloads its own,
# which can hang. Resolved before cd, so a relative path works.
RUNTIME_FILE=${1:-}
if [ -n "$RUNTIME_FILE" ]; then
    if [ ! -f "$RUNTIME_FILE" ]; then
        echo "ERROR: runtime file not found: $RUNTIME_FILE" >&2
        exit 1
    fi
    RUNTIME_FILE=$(readlink -f "$RUNTIME_FILE")
fi

VERSION=$("$SRC_DIR/scripts/extract_version.sh")
OUTPUT_DIR=${MPZ_OUTPUT_DIR:-$HOME/Desktop}
TMP_DIR=$(mktemp -d -t mpz-build-appimage-$(date +%Y-%m-%d-%H-%M-%S)-XXXXX)
cd "$TMP_DIR"

ARCH=$(uname -m)
case "$ARCH" in
    x86_64|aarch64) ;;
    *)
        echo "ERROR: unsupported architecture: $ARCH" >&2
        exit 1
        ;;
esac

# Name-Version-Architecture.AppImage, per the AppImage catalog nomenclature.
SUFFIX="${PACKAGE_VERSION:+-$PACKAGE_VERSION}"
ARTIFACT_NAME=mpz-$VERSION$SUFFIX-$ARCH

find_tool() {
    local candidate
    for candidate in "$1-$ARCH.AppImage" "$1-$ARCH.appimage" "$1"; do
        command -v "$candidate" && return 0
    done
    return 1
}

missing_tool() {
    echo "ERROR: $1 not found on PATH (tried $1-$ARCH.AppImage, $1-$ARCH.appimage, $1)." >&2
    echo "Get it from https://github.com/linuxdeploy/$1/releases/download/continuous/$1-$ARCH.AppImage" >&2
    echo "then chmod +x and add its directory to PATH." >&2
    exit 1
}

LINUXDEPLOY=$(find_tool linuxdeploy) || missing_tool linuxdeploy
# linuxdeploy finds the qt plugin itself; check early to fail before the build.
find_tool linuxdeploy-plugin-qt >/dev/null || missing_tool linuxdeploy-plugin-qt

if [ -n "$RUNTIME_FILE" ]; then
    export LDAI_RUNTIME_FILE="$RUNTIME_FILE"
fi

QMAKE=${QTDIR:+$QTDIR/bin/qmake6}
QMAKE=${QMAKE:-$(command -v qmake6 || true)}
if [ -z "$QMAKE" ] || [ ! -x "$QMAKE" ]; then
    echo "ERROR: qmake6 not found. Set QTDIR or add it to PATH." >&2
    exit 1
fi

echo -e "version:\t$VERSION"
echo -e "source dir:\t$SRC_DIR"
echo -e "build dir:\t$TMP_DIR"
echo -e "output dir:\t$OUTPUT_DIR"

EXTRA_CMAKE_ARGS=()
if [ -n "${QTDIR:-}" ]; then
    EXTRA_CMAKE_ARGS+=("-DCMAKE_PREFIX_PATH=$QTDIR")
fi
if [ -n "${PACKAGE_VERSION:-}" ]; then
    EXTRA_CMAKE_ARGS+=("-DPACKAGE_VERSION=$PACKAGE_VERSION")
fi

# Off by default on Linux, but AppImage users have no package manager to update them.
cmake -DCMAKE_BUILD_TYPE=Release \
      -DENABLE_UPDATE_CHECK=ON \
      -GNinja \
      ${EXTRA_CMAKE_ARGS[@]+"${EXTRA_CMAKE_ARGS[@]}"} \
      "$SRC_DIR"
ninja
test -f ./mpz || { echo "ERROR: build failed, mpz was not produced" >&2; exit 1; }

DESTDIR="$TMP_DIR/AppDir" cmake --install . --prefix /usr

# appimagetool only looks for the old .appdata.xml name.
cp AppDir/usr/share/metainfo/org.mpz_player.mpz.metainfo.xml \
   AppDir/usr/share/metainfo/org.mpz_player.mpz.appdata.xml

export QMAKE
# Lets linuxdeploy find the Qt libs when Qt is not in a system path.
export LD_LIBRARY_PATH="${QTDIR:+$QTDIR/lib:}${LD_LIBRARY_PATH:-}"
export EXTRA_QT_PLUGINS="svg"
export APPIMAGE_EXTRACT_AND_RUN=1

# Qt's multimedia backends, minus gstreamer: its codecs load at runtime, so it
# cannot be bundled. Copied here rather than via the qt plugin, which has no way
# to skip a backend and would fail on the gstreamer libs.
mkdir -p AppDir/usr/plugins
cp -r "$("$QMAKE" -query QT_INSTALL_PLUGINS)/multimedia" AppDir/usr/plugins/
rm -f AppDir/usr/plugins/multimedia/libgstreamermediaplugin.so

# Icon comes from the source tree: cmake installs sizes up to 48px only.
LDAI_OUTPUT="$ARTIFACT_NAME.AppImage" "$LINUXDEPLOY" \
    --appdir AppDir \
    --executable AppDir/usr/bin/mpz \
    --desktop-file AppDir/usr/share/applications/org.mpz_player.mpz.desktop \
    --icon-file "$SRC_DIR/app/resources/icons/256x256/mpz.png" \
    --icon-filename org.mpz_player.mpz \
    --deploy-deps-only AppDir/usr/plugins/multimedia \
    --plugin qt \
    --output appimage

mkdir -p "$OUTPUT_DIR"
rm -f "$OUTPUT_DIR/$ARTIFACT_NAME.AppImage"
cp "$ARTIFACT_NAME.AppImage" "$OUTPUT_DIR/"

echo -e "version:\t$VERSION"
echo -e "source dir:\t$SRC_DIR"
echo -e "build dir:\t$TMP_DIR"
echo -e "artifact:\t$OUTPUT_DIR/$ARTIFACT_NAME.AppImage"
