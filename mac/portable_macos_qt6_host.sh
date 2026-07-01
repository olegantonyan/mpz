#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
SRC_DIR=$(cd "$SCRIPT_DIR/.." && pwd)
VERSION=$("$SRC_DIR/win/_extract_version.sh")
OUTPUT_DIR=${MPZ_OUTPUT_DIR:-$HOME/Desktop}
TMP_DIR=$(mktemp -d -t mpz-build-macos-qt6-$(date +%Y-%m-%d-%H-%M-%S)-XXXXX)
cd "$TMP_DIR"

SUFFIX="${PACKAGE_VERSION:+-$PACKAGE_VERSION}"
ARTIFACT_NAME=mpz-$VERSION$SUFFIX-macos-universal-qt6
# Name shown in Finder / Applications / "Open With" — this is the on-disk
# bundle name, which is what macOS displays (CFBundleDisplayName only applies
# when it matches the filesystem name). Kept separate from the versioned
# DMG/artifact file name on purpose.
APP_NAME="mpz music player"

echo -e "version:\t$VERSION"
echo -e "source dir:\t$SRC_DIR"
echo -e "build dir:\t$TMP_DIR"
echo -e "output dir:\t$OUTPUT_DIR"

EXTRA_CMAKE_ARGS=()
if [ -n "${PACKAGE_VERSION:-}" ]; then
    EXTRA_CMAKE_ARGS+=("-DPACKAGE_VERSION=$PACKAGE_VERSION")
fi

# Sparkle is built by cmake (custom target) from the vendored source in one build;
# we only opt in here and note where cmake writes the framework so we can embed it
# after macdeployqt below.
SPARKLE_FRAMEWORK=""
if [ -d "$SRC_DIR/3rdparty/Sparkle-2.9.3/Sparkle.xcodeproj" ]; then
    EXTRA_CMAKE_ARGS+=("-DENABLE_SPARKLE=ON")
    SPARKLE_FRAMEWORK="$TMP_DIR/sparkle/Build/Products/Release/Sparkle.framework"
    echo -e "Sparkle:\tenabled (cmake builds the framework from vendored source)"
else
    echo -e "Sparkle:\tdisabled (no vendored source)"
fi

cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
      -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
      -GNinja \
      ${EXTRA_CMAKE_ARGS[@]+"${EXTRA_CMAKE_ARGS[@]}"} \
      "$SRC_DIR"
ninja

MACDEPLOYQT=${QTDIR:+$QTDIR/bin/macdeployqt}
MACDEPLOYQT=${MACDEPLOYQT:-$(command -v macdeployqt || true)}
if [ -z "$MACDEPLOYQT" ] || [ ! -x "$MACDEPLOYQT" ]; then
    echo "ERROR: macdeployqt not found. Set QTDIR or add it to PATH." >&2
    exit 1
fi

"$MACDEPLOYQT" ./mpz.app -always-overwrite -verbose=1

# Embed the cmake-built Sparkle.framework after macdeployqt — doing it before
# would let macdeployqt (which sees the main binary's @rpath/Sparkle dependency)
# rewrite the framework's nested code. Before codesign so the ad-hoc sign covers
# it; ditto preserves symlinks and executable bits.
if [ -d "$SPARKLE_FRAMEWORK" ]; then
    mkdir -p ./mpz.app/Contents/Frameworks
    ditto "$SPARKLE_FRAMEWORK" ./mpz.app/Contents/Frameworks/Sparkle.framework
fi

# Ad-hoc codesign: enough for Gatekeeper "right-click → Open" bypass,
# not enough for redistribution without warnings. Replace later with
# Developer ID signing when an Apple Developer account exists.
codesign --deep --force --sign - ./mpz.app

mkdir -p "$OUTPUT_DIR"
rm -rf "$OUTPUT_DIR/$APP_NAME.app" "$OUTPUT_DIR/$ARTIFACT_NAME.dmg"
cp -R ./mpz.app "$OUTPUT_DIR/$APP_NAME.app"
hdiutil create -volname "mpz" \
               -srcfolder "$OUTPUT_DIR/$APP_NAME.app" \
               -ov -format UDZO \
               "$OUTPUT_DIR/$ARTIFACT_NAME.dmg"

echo -e "version:\t$VERSION"
echo -e "source dir:\t$SRC_DIR"
echo -e "build dir:\t$TMP_DIR"
echo -e "artifact:\t$OUTPUT_DIR/$ARTIFACT_NAME.dmg"
