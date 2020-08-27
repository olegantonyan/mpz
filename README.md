[![GPLv3 License](https://img.shields.io/badge/License-GPL%20v3-yellow.svg)](https://opensource.org/licenses/)

# WIP misic player

https://olegantonyan.github.io/mpz/

## Installation

#### Linux

Use open build service repositories for openSUSE, Ubuntu, Fedora and other popular distros: https://software.opensuse.org//download.html?project=home%3Aoleg_antonyan&package=mpz

#### From sources

Dependencies: gcc, make, qt development headers (libqt5-qtbase-devel libqt5-qtmultimedia-devel libqt5-qtx11extras-devel).
Packages' names may differ in different distros.

```
git clone git@github.com:olegantonyan/mpz.git
cd mpz
mkdir build
cd build
qmake-qt5 ..
make -j8
# now you now use app/mpz binary directly
# optionally, install to /usr/bin with icon and desktop file, as root:
make install
```

#### Windows

Grab static binaries from releases page: https://github.com/olegantonyan/mpz/releases
