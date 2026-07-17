#!/usr/bin/env ruby

# required ENV variables: GITHUB_SHA, AUR_SSH_PRIVATE_KEY, SRCINFO, PKGBUILD, PKGNAME
# set DRY_RUN=1 to render and print PKGBUILD/.SRCINFO and exit without touching AUR
# (GITHUB_SHA and AUR_SSH_PRIVATE_KEY are not required then, and no network is used)

require 'erb'
require 'digest'
require 'open-uri'
require 'tmpdir'


puts "PWD: #{::Dir.pwd}"

dry_run = !::ENV['DRY_RUN'].to_s.empty?
puts "DRY RUN: nothing will be pushed to aur" if dry_run

commit_hash = ::ENV['GITHUB_SHA'] || (dry_run ? `git rev-parse HEAD`.strip : raise('no GITHUB_SHA'))
aur_ssh_key = ENV['AUR_SSH_PRIVATE_KEY'] || (dry_run ? nil : raise('no AUR_SSH_PRIVATE_KEY'))
pkgname = ::ENV['PKGNAME'] || (raise 'no PKGNAME')
pkgbuild_path = ::ENV['PKGBUILD'] || (raise 'no PKGBUILD')
srcinfo_path = ::ENV['SRCINFO'] || (raise 'no SRCINFO')

puts "Commit hash: #{commit_hash}"

source = "https://github.com/olegantonyan/mpz/archive/#{commit_hash}.zip"
puts "Source tarball: #{source}"

aur_repo = "ssh://aur@aur.archlinux.org/#{pkgname}.git"
pkgrel = `git log --oneline $(git describe --tags --abbrev=0).. | wc -l`.strip
pkgver = /project\(mpz VERSION (.+) LANGUAGES/.match(::File.read('CMakeLists.txt'))[1].to_s.strip
sha256sums = dry_run ? 'SKIP' : ::Digest::SHA256.hexdigest(::URI.open(source).read)
author_name = `git --no-pager log -1 --pretty=format:'%an'`.strip
author_email = `git --no-pager log -1 --pretty=format:'%ae'`.strip
last_commit_hash = `git rev-parse --short HEAD`.strip
last_commit_message = `git --no-pager log -1 --pretty=format:'%s'`.strip
last_commit_url = "https://github.com/olegantonyan/mpz/commit/#{last_commit_hash}"

puts "git describe --tags: #{`git describe --tags`}"

raise "Commits mismtach: #{commit_hash} != #{last_commit_hash}" unless commit_hash.start_with?(last_commit_hash)

pkgbuild = ::ERB.new(::File.read(pkgbuild_path), trim_mode: '-').result_with_hash(
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

srcinfo = ::ERB.new(::File.read(srcinfo_path), trim_mode: '-').result_with_hash(
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

if dry_run
  puts "DRY RUN: skipping aur clone/commit/push"
  exit 0
end

::Dir.mktmpdir do |d|
  puts "TMP DIR: #{d}"

  keyfile = "#{d}/ssh_key"
  ::File.open(keyfile, 'w') { |f| f.write(aur_ssh_key) }
  `chmod 400 #{keyfile}`

  git_ssh = "GIT_SSH_COMMAND='ssh -i #{keyfile} -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no'"

  puts "cloning aur repo..."
  clone_cmd = "#{git_ssh} git clone #{aur_repo}"
  puts clone_cmd
  system("cd #{d} && #{clone_cmd}") || raise('aur clone failed')

  ::File.open("#{d}/#{pkgname}/PKGBUILD", 'w') { |f| f.write(pkgbuild) }
  ::File.open("#{d}/#{pkgname}/.SRCINFO", 'w') { |f| f.write(srcinfo) }

  raise '.SRCINFO file mismatch' if ::File.read("#{d}/#{pkgname}/.SRCINFO") != srcinfo
  raise 'PKGBUILD file mismatch' if ::File.read("#{d}/#{pkgname}/PKGBUILD") != pkgbuild

  `git config --global user.email "#{author_email}"`
  `git config --global user.name "#{author_name}"`

  puts "pushing changes to aur..."
  system("cd #{d}/#{pkgname} && git add . --all") || raise('aur git add failed')
  if `cd #{d}/#{pkgname} && git status --porcelain`.strip.empty?
    puts "nothing to commit, aur is already up to date"
  else
    system("cd #{d}/#{pkgname} && git commit -m \"#{last_commit_message} (#{pkgver}-#{pkgrel} #{last_commit_url})\"") || raise('aur commit failed')
    system("cd #{d}/#{pkgname} && #{git_ssh} git push") || raise('aur push failed')
  end
end
