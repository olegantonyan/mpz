#!/bin/bash

QT_PATH=/e/Qt/6.8.0/mingw_64/
TOOLCHAIN_PATH=/e/Qt/Tools/mingw1310_64/bin
INSTALLER_FRAMEWORK_PATH=/e/Qt/Tools/QtInstallerFramework/4.8/bin

export PATH=$PATH:$TOOLCHAIN_PATH:$QT_PATH/bin/:$INSTALLER_FRAMEWORK_PATH

echo -e "Qt path:\t$QT_PATH"
echo -e "Toolchain:\t$TOOLCHAIN_PATH"
echo -e "Installer:\t$INSTALLER_FRAMEWORK_PATH"
