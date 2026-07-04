#!/bin/bash

if [ -z "${QTDIR:-}" ]; then
    QT_VERSION=6.11.1
    INSTALLER_FRAMEWORK_VERSION=4.11

    QT_INSTALL_PATH=/e/Qt

    QTDIR=$QT_INSTALL_PATH/$QT_VERSION/mingw_64/
    TOOLCHAIN_PATH=$QT_INSTALL_PATH/Tools/mingw1310_64/bin
    INSTALLER_FRAMEWORK_PATH=$QT_INSTALL_PATH/Tools/QtInstallerFramework/$INSTALLER_FRAMEWORK_VERSION/bin
    CMAKE_PATH=$QT_INSTALL_PATH/Tools/CMake_64/bin
    NINJA_PATH=$QT_INSTALL_PATH/Tools/Ninja

    export PATH=$PATH:$TOOLCHAIN_PATH:$QTDIR/bin/:$INSTALLER_FRAMEWORK_PATH:$CMAKE_PATH:$NINJA_PATH
    export QTDIR

    echo -e "Qt path:\t$QTDIR"
    echo -e "Toolchain:\t$TOOLCHAIN_PATH"
    echo -e "CMake:\t$CMAKE_PATH"
    echo -e "Ninja:\t$NINJA_PATH"
    echo -e "Installer:\t$INSTALLER_FRAMEWORK_PATH"
fi

# Copy the MSVC C runtime (vcruntime140*.dll, msvcp140*.dll, ...) next to the
# app so it runs on machines without the VC++ redistributable installed. Used by
# the MSVC build scripts (x64 and arm64). This is Microsoft's supported
# "local deployment" of the CRT: windeployqt --compiler-runtime is not usable
# for a portable app because on MSVC it only drops the vc_redist.<arch>.exe
# installer (which the user would have to run), and on arm64 it is unreliable.
# Requires the MSVC environment (VCToolsRedistDir) to be set, e.g. by
# "vcvarsall <arch>" / the ilammy/msvc-dev-cmd action.
# $1: destination dir. $2: redist arch subdir (x64 or arm64).
copy_vc_runtime() {
    local dest="$1"
    local arch="$2"
    if [ -z "$arch" ]; then
        echo "ERROR: copy_vc_runtime requires an arch argument (x64 or arm64)" >&2
        exit 1
    fi
    if [ -z "${VCToolsRedistDir:-}" ]; then
        echo "ERROR: VCToolsRedistDir is not set; MSVC environment not initialized" >&2
        exit 1
    fi
    local redist crt_dir
    redist=$(cygpath -u "$VCToolsRedistDir")
    crt_dir=$(find "$redist/$arch" -maxdepth 1 -type d -iname 'Microsoft.VC*.CRT' | head -1)
    if [ -z "$crt_dir" ]; then
        echo "ERROR: could not find Microsoft.VC*.CRT under $redist/$arch" >&2
        exit 1
    fi
    echo "bundling VC++ $arch runtime from: $crt_dir"
    cp "$crt_dir"/*.dll "$dest"/
}
