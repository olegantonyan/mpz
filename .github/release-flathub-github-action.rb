#!/usr/bin/env ruby

# required ENV variables: RELEASE_TAG, FLATHUB_TOKEN
# set DRY_RUN=1 to clone flathub anonymously, print the resulting diff and exit without pushing
# (FLATHUB_TOKEN is not required then, and nothing is written to flathub)

require 'yaml'
require 'tmpdir'

FLATHUB_REPO = 'flathub/org.mpz_player.mpz'.freeze
MANIFEST = 'org.mpz_player.mpz.yml'.freeze

puts "PWD: #{::Dir.pwd}"

dry_run = !::ENV['DRY_RUN'].to_s.empty?
puts "DRY RUN: nothing will be pushed to flathub" if dry_run

release_tag = ::ENV['RELEASE_TAG'].to_s.strip
raise 'no RELEASE_TAG' if release_tag.empty?
flathub_token = ::ENV['FLATHUB_TOKEN'] || (dry_run ? nil : raise('no FLATHUB_TOKEN'))

commit_hash = `git rev-list -n 1 #{release_tag}`.strip
raise "cannot resolve tag #{release_tag}" unless $?.success? && !commit_hash.empty?

cmakelists = /project\(mpz VERSION (.+) LANGUAGES/.match(`git show #{release_tag}:CMakeLists.txt`)
raise "no project version in CMakeLists.txt at #{release_tag}" unless cmakelists

version = cmakelists[1].to_s.strip

puts "Release tag: #{release_tag}"
puts "Commit hash: #{commit_hash}"
puts "CMakeLists version: #{version}"

raise "Version mismatch: tag #{release_tag} != CMakeLists #{version}" unless release_tag == version

def substitute(text, key, value)
  pattern = /^([ \t]*)#{key}:[^\n]*$/
  matches = text.scan(pattern).size
  raise "expected exactly one '#{key}:' line in #{MANIFEST}, found #{matches}" unless matches == 1

  text.sub(pattern, "\\1#{key}: #{value}")
end

::ENV['GH_TOKEN'] = flathub_token unless dry_run

::Dir.mktmpdir do |d|
  puts "TMP DIR: #{d}"

  clone_url = if dry_run
                "https://github.com/#{FLATHUB_REPO}.git"
              else
                "https://x-access-token:#{flathub_token}@github.com/#{FLATHUB_REPO}.git"
              end

  puts "cloning flathub repo..."
  system("cd #{d} && git clone #{clone_url} flathub") || raise('flathub clone failed')

  manifest_path = "#{d}/flathub/#{MANIFEST}"

  current = ::YAML.load_file(manifest_path).dig('modules', 0, 'sources', 0)
  if current['tag'].to_s == release_tag && current['commit'].to_s == commit_hash
    puts "flathub is already up to date"
    next
  end

  manifest = ::File.read(manifest_path)

  # unquoted two-component versions like 2.2 would parse as a YAML float
  manifest = substitute(manifest, 'tag', "'#{release_tag}'")
  manifest = substitute(manifest, 'commit', commit_hash)

  ::File.write(manifest_path, manifest)

  puts "***** #{MANIFEST} *****"
  puts manifest
  puts "***** END OF #{MANIFEST} *****"

  source = ::YAML.load_file(manifest_path).dig('modules', 0, 'sources', 0)
  raise "unexpected source type: #{source['type']}" unless source['type'] == 'git'
  raise "tag mismatch: #{source['tag']}" unless source['tag'] == release_tag
  raise "commit mismatch: #{source['commit']}" unless source['commit'] == commit_hash

  raise 'manifest unchanged' if `cd #{d}/flathub && git status --porcelain`.strip.empty?

  if dry_run
    puts "***** DIFF *****"
    system("cd #{d}/flathub && git --no-pager diff")
    puts "DRY RUN: skipping flathub branch push and pull request"
    next
  end

  branch = "update-#{release_tag}"
  release_url = "https://github.com/olegantonyan/mpz/releases/tag/#{release_tag}"

  `git config --global user.email "41898282+github-actions[bot]@users.noreply.github.com"`
  `git config --global user.name "github-actions[bot]"`

  puts "pushing branch #{branch} to flathub..."
  system("cd #{d}/flathub && git checkout -B #{branch}") || raise('flathub checkout failed')
  system("cd #{d}/flathub && git commit -a -m 'update to #{release_tag} release'") || raise('flathub commit failed')
  system("cd #{d}/flathub && git push --force origin #{branch}") || raise('flathub push failed')

  existing = `gh pr list --repo #{FLATHUB_REPO} --head #{branch} --state open --json url --jq '.[0].url'`.strip
  pr_url = if existing.empty?
             puts "creating pull request..."
             out = `gh pr create --repo #{FLATHUB_REPO} --base master --head #{branch} --title 'update to #{release_tag} release' --body '#{release_url}'`.strip
             raise 'gh pr create failed' unless $?.success?

             out
           else
             puts "pull request already open, updated in place"
             existing
           end

  puts "pull request: #{pr_url}"
  ::File.open(::ENV['GITHUB_OUTPUT'], 'a') { |f| f.puts("pr_url=#{pr_url}") } if ::ENV['GITHUB_OUTPUT']
end
