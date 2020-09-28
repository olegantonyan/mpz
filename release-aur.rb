#!/usr/bin/env ruby

require 'erb'
require 'digest'
require 'open-uri'
require 'tmpdir'

PKGBUILD = <<~HEREDOC
# Maintainer: Oleg Antonyan <oleg.b.antonyan@gmail.com>
# Contributor: Oleg Antonyan <oleg.b.antonyan@gmail.com>

pkgname=mpz
pkgver=<%= pkgver %>
pkgrel=<%= pkgrel %>
pkgdesc='Music player for the large local collections'
arch=('x86_64')
url="https://github.com/olegantonyan/mpz"
license=('GPL3')
depends=('qt5-multimedia' 'qt5-x11extras' 'hicolor-icon-theme')
provides=('mpz')
source=(<%= source %>)
sha256sums=('<%= sha256sums %>')

build() {
    cd $pkgname-$pkgver

    rm -rf build
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
HEREDOC

Dockerfile = <<~HEREDOC
FROM archlinux:latest

RUN pacman -Sy base-devel --noconfirm
RUN useradd -m build

CMD ["su", "-", "build", "-c", "makepkg --printsrcinfo ~/build/PKGBUILD"]
HEREDOC

def aur_repo
  'ssh://aur@aur.archlinux.org/mpz.git'
 end

def sha256sums
  content = open(source).read
  ::Digest::SHA256.hexdigest(content)
end

def pkgrel
  return 1 # NOTE: not using tags here
  `git log --oneline $(git describe --tags --abbrev=0).. | wc -l`.strip
end

def pkgver
  txt = ::File.read('version.pri')
  /(?<=").+(?=\\\\\\\")/.match(txt).to_s.strip
end

def source
  "https://github.com/olegantonyan/mpz/archive/#{pkgver}.tar.gz"
end

pkgbuild = ::ERB.new(PKGBUILD, nil, '-').result_with_hash(
  pkgver: pkgver,
  pkgrel: pkgrel,
  sha256sums: sha256sums,
  source: source
)
puts "***** PKGBUILD *****"
puts pkgbuild
puts "***** END OF PKGBUILD *****"

::Dir.mktmpdir do |d|
  puts "temp dir: #{d}"
  ::File.open("#{d}/Dockerfile", 'w') { |f| f.write(Dockerfile) }
  `cd #{d} && docker build -t arch-makepkg .`

  `cd #{d} && git clone #{aur_repo}`
  ::File.open("#{d}/mpz/PKGBUILD", 'w') { |f| f.write(pkgbuild) }

  `docker run -it --rm -v #{d}/mpz:/home/build arch-makepkg > #{d}/mpz/.SRCINFO`

  srcinfo = ::File.read("#{d}/mpz/.SRCINFO")
  puts "***** .SRCINFO *****"
  puts srcinfo
  puts "***** END OF .SRCINFO *****"

  #`cd #{d}/mpz && git add . --all && git commit -m "release version #{pkgver}" && git push`
end
