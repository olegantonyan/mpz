#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
SRC_DIR=$(cd "$SCRIPT_DIR/.." && pwd)
VERSION=$("$SRC_DIR/win/_extract_version.sh")
OUTPUT_DIR=${MPZ_OUTPUT_DIR:-$HOME/Desktop}
TMP_DIR=$(mktemp -d -t mpz-build-macos-qt6-$(date +%Y-%m-%d-%H-%M-%S)-XXXXX)
cd "$TMP_DIR"

SUFFIX="${PACKAGE_VERSION:+-$PACKAGE_VERSION}"
ARTIFACT_NAME=mpz-$VERSION$SUFFIX-macos-universal
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

# Ad-hoc codesign: enough for Gatekeeper "right-click → Open" bypass,
# not enough for redistribution without warnings. Replace later with
# Developer ID signing when an Apple Developer account exists.
codesign --deep --force --sign - ./mpz.app

mkdir -p "$OUTPUT_DIR"
rm -rf "$OUTPUT_DIR/$APP_NAME.app" "$OUTPUT_DIR/$ARTIFACT_NAME.dmg"
cp -R ./mpz.app "$OUTPUT_DIR/$APP_NAME.app"

VOL_NAME="mpz music player"
STAGE_DIR="$TMP_DIR/dmg_stage"
RW_DMG="$TMP_DIR/rw.dmg"
MOUNT_POINT="/Volumes/$VOL_NAME"

rm -rf "$STAGE_DIR"
mkdir -p "$STAGE_DIR"
cp -R ./mpz.app "$STAGE_DIR/$APP_NAME.app"
ln -s /Applications "$STAGE_DIR/Applications"

hdiutil create -volname "$VOL_NAME" -srcfolder "$STAGE_DIR" \
               -ov -format UDRW "$RW_DMG"

[ -d "$MOUNT_POINT" ] && hdiutil detach "$MOUNT_POINT" -force || true
for attempt in 1 2 3; do
    if hdiutil attach "$RW_DMG" -mountpoint "$MOUNT_POINT" -nobrowse -noverify -noautoopen; then
        break
    fi
    echo "attach failed (attempt $attempt), retrying..." >&2
    sleep 2
done

osascript <<EOF || echo "warning: Finder layout failed; shipping default icon layout" >&2
tell application "Finder"
    tell disk "$VOL_NAME"
        open
        set current view of container window to icon view
        set toolbar visible of container window to false
        set statusbar visible of container window to false
        set the bounds of container window to {200, 120, 860, 520}
        set theViewOptions to the icon view options of container window
        set arrangement of theViewOptions to not arranged
        set icon size of theViewOptions to 128
        set position of item "$APP_NAME.app" of container window to {170, 190}
        set position of item "Applications" of container window to {490, 190}
        update without registering applications
        delay 1
        close
    end tell
end tell
EOF

sync
hdiutil detach "$MOUNT_POINT" -force
hdiutil convert "$RW_DMG" -format UDZO -imagekey zlib-level=9 \
               -ov -o "$OUTPUT_DIR/$ARTIFACT_NAME.dmg"
rm -rf "$RW_DMG" "$STAGE_DIR"

echo -e "version:\t$VERSION"
echo -e "source dir:\t$SRC_DIR"
echo -e "build dir:\t$TMP_DIR"
echo -e "artifact:\t$OUTPUT_DIR/$ARTIFACT_NAME.dmg"
