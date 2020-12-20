[![GPLv3 License](https://img.shields.io/badge/License-GPL%20v3-yellow.svg)](https://opensource.org/licenses/)

# Music player for big local collections

![image](https://raw.githubusercontent.com/olegantonyan/mpz/gh-pages/images/mpz-workflow.apng)

If you like to organize your music in folders, then this player might be for you. It doesn't try to index all the files into a library, but rather treats your files and folders as a library and provides a convenient way to create playlists directly from folders. Similar to Foobar2000's Album List, but it's not an attempt to clone.

More screenshots here: https://olegantonyan.github.io/mpz/

## Why?

In about 15 years author couldn't find a suitable player for Linux. Foobar2000 works in Wine, but this solution is not perfect either. This player is an attempt to create the "perfect" player for the author. It doesn't have anything "breakthrough", it just gets the job done. The main feature is 3-columns UI and the way you manage playlists. Chose library folders, middle-click on a folder and a playlist will be created from this folder.

Why "big local collections"? "Local" opposed to streaming services (which are fine, but this player's goal is to play music you own, not rent), "big" means it's big enough so managing it becomes hard.

## Features

- 3-columns UI which allows you to quickly create playlists from folders and switch between playlists;
- Built with C++/Qt - fast and responsive native UI;
- Supports internet radio in `m3u` and `pls` playlists formats;
- Supports CUE sheets;
- Supports MPRIS on Linux for remote control;
- Configuration in 2 yaml files: one for global (portable between computers) and one local (for settings specific to the current installation).

## Limitations

- Uses external codecs installed on your OS (through QtMultimedia, using GStreamer backend on Linux);
- Lacks some "expected" features like tracks rearranging within playlist;
- Global hotkeys don't work in Wayland.

## Installation

#### openSUSE, Debian, Fedora, Ubuntu, CentOS, Mageia

Use Open Build Service repositories: https://software.opensuse.org//download.html?project=home%3Aoleg_antonyan&package=mpz

#### Arch

Use AUR package: https://aur.archlinux.org/packages/mpz/

```
git clone https://aur.archlinux.org/mpz.git
cd mpz
makepkg -si
```

Please, beware of https://aur.archlinux.org/packages/mpz-git/. This package is not maintained by the author and may be outdated. The only maintained AUR package is this https://aur.archlinux.org/packages/mpz/.

#### From sources

Dependencies: gcc, make, qt development headers (libqt5-qtbase-devel libqt5-qtmultimedia-devel libqt5-qtx11extras-devel).
Packages' names may differ in different distros.

```
git clone git@github.com:olegantonyan/mpz.git
cd mpz
mkdir build
cd build
qmake-qt5 CONFIG+=release ..
make -j`nproc`
# now you now use app/mpz binary directly
# optionally, install to /usr:
sudo make install
```

#### Windows

Use static binaries from releases page: https://github.com/olegantonyan/mpz/releases
You'll need codecs installed on your system, for example, [K-Lite Codec Pack](https://www.codecguide.com/download_kl.htm).
NOTE: Windows binaries aren't tested as thoroughly as Linux and may contain Windows-specific bugs.

#### MacOS

It should be possible to build mpz for Mac, but I don't have a hardware to test it. If you're interested in Mac builds - drop an issue here https://github.com/olegantonyan/mpz/issues/new

## Configuration

The default config location on Linux is `~/.config/mpz`, on Windows - `C:/Users/$USERNAME/AppData/Local/mpz/mpz`. There are 2 files:
- `local.yml` - for the settings specific to this computer, like windows' sizes, playlists, etc;
- `global.yml` - for portable settings that make sense to share between computers.

Some config options can be changed only by editing config files:

- `stream_buffer_size` in `global.yml` - minimal stream buffer size in bytes. The default is 128KB;
- `single_instance` in `global.yml` - when `true` the player will reuse 1 instance, launching another instance with files as command line arguments will send these files to running instance as a new playlist;
- `single_instance_ipc_port` in `global.yml` - single instance functionality uses TCP socket, this option allows you to specify a port;
- `playback_log_size` in `global.yml` - max size of playback log, default is 100;
- `columns_config` in `global.yml` - configure columns in playlist section, more on this below;

If you messed up any of the config opions you can remove it completly (or even remove the whole file) and it will reset to default.

#### Columns config

You can change th default columns in the playlist view via `columns_config` option in `global.yml` file.

This config option does not (yet) have UI to change it so you have to edit config file. The defaults are:

```
columns_config:
  - align: left
    field: artist
    stretch: false
    width_percent: 28
  - align: left
    field: album
    stretch: false
    width_percent: 28
  - align: left
    field: title
    stretch: false
    width_percent: 28
  - align: right
    field: year
    stretch: false
    width_percent: 5
  - align: right
    field: length
    stretch: true
    width_percent: 0
```

Available fields: artist, album, title, year, length, path, url, sample_rate, bitrate, channels, track_number, format, filename.

Availble alignments: left, right.

The sum of `width_percent` of all columns must add up to 100 or below. Sometimes it has to be below 100 to get rid of horizontal scroll, this may happen due to padding and few extra pixels in you desktop theme.

`stretch` will stretch the column to fit the window width to the right. It's advised to have the last column stretched and the sum of all `width_percent` below 100, but you can experiment with it and see how looks on your desktop.

## Hotkeys

mpz supports global media keys (typically found on a keyboard play/pause/stop/...) as well as non-global shortcuts:
- Ctrl+L - open playback log;
- Alt+T - next track;
- Alt+R - previous track;
- Alt+W - pause;
- Alt+Q - stop;
- Alt+E - play;
- Alt+3 - focus playlists filter;
- Alt+2 - focus playlists filter;
- Alt+1 - focus library (directory tree) filter;
- Ctrl+3 - focus playlist;
- Ctrl+2 - focus playlists;
- Ctrl+1 - focus library (directory tree);
- Ctrl+Q - exit;
- Ctrl+S - open sort menu;

## [Changelog](https://github.com/olegantonyan/mpz/blob/master/CHANGELOG.md)
