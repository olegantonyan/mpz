[![GPLv3 License](https://img.shields.io/badge/License-GPL%20v3-yellow.svg)](https://opensource.org/licenses/)

# WIP misic player

## Installation

#### openSUSE (Tumbleweed, 15.2, 15.1)

https://software.opensuse.org//download.html?project=home%3Aoleg_antonyan&package=mpz

Use OBS repository:
```
zypper ar -f obs://home:oleg_antonyan home:oleg_antonyan
zypper in mpz
```

To update:
```
zypper up mpz
```

Or regualar update on Tumbleweed should pick up a new version:
```
zypper dup
```

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
