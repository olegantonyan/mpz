# Maintainer: Oleg Antonyan <oleg.b.antonyan@gmail.com>
# Contributor: Oleg Antonyan <oleg.b.antonyan@gmail.com>

pkgname=mpz
pkgver=0.0.7
pkgrel=1
pkgdesc='Music player for the large local collections'
arch=('x86_64')
url="https://github.com/olegantonyan/mpz/"
license=('GPL3')
depends=('')
makedepends=('gcc' 'make' 'libqt5-qtbase-devel' 'libqt5-qtmultimedia-devel' 'libqt5-qtx11extras-devel')

provides=('mpz')
source=($pkgname-$pkgver.tar.gz)

build() {
    cd $pkgname-$pkgver

    mkdir build
    cd build
    qmake-qt5 CONFIG+=release ..
    make
}

package() {
    cd $pkgname-$pkgver
    cd build
    make install INSTALL_ROOT=$pkgdir
}
