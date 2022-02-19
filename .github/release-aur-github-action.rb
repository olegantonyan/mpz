#!/usr/bin/env ruby

# required ENV variables: GITHUB_SHA, AUR_SSH_PRIVATE_KEY

require 'erb'
require 'digest'
require 'open-uri'
require 'tmpdir'

PKGBUILD = <<~HEREDOC
# Maintainer: Oleg Antonyan <oleg.b.antonyan@gmail.com>
# Contributor: Oleg Antonyan <oleg.b.antonyan@gmail.com>

pkgname=<%= pkgname %>
pkgver=<%= pkgver %>
pkgrel=<%= pkgrel %>
pkgdesc='Music player for the large local collections'
arch=('x86_64')
url="https://github.com/olegantonyan/mpz"
license=('GPL3')
depends=('qt5-multimedia' 'qt5-x11extras' 'hicolor-icon-theme')
provides=('<%= pkgname %>')
source=("$pkgname-$pkgver-$pkgrel.zip::<%= source %>")
sha256sums=('<%= sha256sums %>')

build() {
    cd $pkgname-<%= commit_hash %>

    rm -rf build
    mkdir build
    cd build
    qmake-qt5 CONFIG+=release ..
    make
}

package() {
    cd $pkgname-<%= commit_hash %>

    cd build
    make install INSTALL_ROOT=$pkgdir
}
HEREDOC

SRCINFO =<<~HEREDOC
pkgbase = <%= pkgname %>
	pkgdesc = Music player for the large local collections
	pkgver = <%= pkgver %>
	pkgrel = <%= pkgrel %>
	url = https://github.com/olegantonyan/mpz
	arch = x86_64
	license = GPL3
	depends = qt5-multimedia
	depends = qt5-x11extras
	depends = hicolor-icon-theme
	provides = <%= pkgname %>
	source = <%= pkgname %>-<%= pkgver %>-<%= pkgrel %>.zip::<%= source %>
	sha256sums = <%= sha256sums %>

pkgname = <%= pkgname %>
HEREDOC

puts "PWD: #{::Dir.pwd}"
commit_hash = ::ENV['GITHUB_SHA'] || (raise 'no GITHUB_SHA')
aur_ssh_key = ENV['AUR_SSH_PRIVATE_KEY'] || (raise 'no AUR_SSH_PRIVATE_KEY')
puts "Commit hash: #{commit_hash}"

source = "https://github.com/olegantonyan/mpz/archive/#{commit_hash}.zip"
puts "Source tarball: #{source}"

aur_repo = 'ssh://aur@aur.archlinux.org/mpz.git'

pkgname = 'mpz'
pkgrel = `git log --oneline $(git describe --tags --abbrev=0).. | wc -l`.strip
pkgver = /(?<=").+(?=\\\\\\\")/.match(::File.read('version.pri')).to_s.strip
sha256sums = ::Digest::SHA256.hexdigest(::URI.open(source).read)
author_name = `git --no-pager log -1 --pretty=format:'%an'`.strip
author_email = `git --no-pager log -1 --pretty=format:'%ae'`.strip
last_commit_hash = `git rev-parse --short HEAD`.strip
last_commit_message = `git --no-pager log -1 --pretty=format:'%s'`.strip
last_commit_url = "https://github.com/olegantonyan/mpz/commit/#{last_commit_hash}"

puts "git describe --tags: #{`git describe --tags`}"

raise "Commits mismtach: #{commit_hash} != #{last_commit_hash}" unless commit_hash.start_with?(last_commit_hash)

pkgbuild = ::ERB.new(PKGBUILD, nil, '-').result_with_hash(
  pkgname: pkgname,
  pkgver: pkgver,
  pkgrel: pkgrel,
  sha256sums: sha256sums,
  source: source,
  commit_hash: commit_hash
)
puts "***** PKGBUILD *****"
puts pkgbuild
puts "***** END OF PKGBUILD *****"

srcinfo = ::ERB.new(SRCINFO, nil, '-').result_with_hash(
  pkgname: pkgname,
  pkgver: pkgver,
  pkgrel: pkgrel,
  sha256sums: sha256sums,
  source: source,
  commit_hash: commit_hash
)
puts "***** SRCINFO *****"
puts srcinfo
puts "***** END OF SRCINFO *****"

::Dir.mktmpdir do |d|
  puts "TMP DIR: #{d}"

  keyfile = "#{d}/ssh_key"
  ::File.open(keyfile, 'w') { |f| f.write(aur_ssh_key) }
  `chmod 400 #{keyfile}`

  git_ssh = "GIT_SSH_COMMAND='ssh -i #{keyfile} -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no'"

  puts "cloning aur repo..."
  clone_cmd = "#{git_ssh} git clone --branch master #{aur_repo}"
  puts clone_cmd
  `cd #{d} && #{clone_cmd}`

  ::File.open("#{d}/mpz/PKGBUILD", 'w') { |f| f.write(pkgbuild) }
  ::File.open("#{d}/mpz/.SRCINFO", 'w') { |f| f.write(srcinfo) }

  raise '.SRCINFO file mismatch' if ::File.read("#{d}/mpz/.SRCINFO") != srcinfo
  raise 'PKGBUILD file mismatch' if ::File.read("#{d}/mpz/PKGBUILD") != pkgbuild

  `git config --global user.email "#{author_email}"`
  `git config --global user.name "#{author_name}"`

  puts "pushing changes to aur..."
  `cd #{d}/mpz && git add . --all && git commit -m "#{last_commit_message} (#{pkgver}-#{pkgrel} #{last_commit_url})" && #{git_ssh} git push`
end
