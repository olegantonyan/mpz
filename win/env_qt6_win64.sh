#!/bin/bash

QT_VERSION=6.8.1
INSTALLER_FRAMEWORK_VERSION=4.8

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
