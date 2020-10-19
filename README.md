[![GPLv3 License](https://img.shields.io/badge/License-GPL%20v3-yellow.svg)](https://opensource.org/licenses/)

# Music player for big local collections

https://olegantonyan.github.io/mpz/

![image](https://raw.githubusercontent.com/olegantonyan/mpz/gh-pages/images/mpz-workflow.apng)

If you like to organize your music in folders, then this player might be for you. It doesn't try to index all the files into a library, but rather treats your files and folders as a library and provides a convenient way to create playlists directly from folders. Similar to Foobar2000's Album List, but it's not an attempt to clone.

## Why?

In about 15 years author couldn't find a suitable player for Linux. Foobar2000 works in Wine, but this solution is not perfect either. This player is an attempt to create the "perfect" player for the author. It doesn't have anything "breakthrough", it just gets the job done. The main feature is 3-columns UI and the way you manage playlists. Chose library folders, middle-click on a folder and a playlist will be created from this folder.

Why "big local collections"? "Local" opposed to streaming services (which are fine, but this player's goal is to play music you own, not rent), "big" means it's big enough so managing it becomes hard.

## Features

- 3-columns UI which allows you to quickly create playlists from folders and switch between playlists;
- Built with Qt/C++ (yes, you can still do it in 2020) - fast and responsive native UI;
- Supports internet radio in `m3u` and `pls` playlists formats;
- Supports CUE sheets;
- Supports MPRIS on Linux for remote control;
- Configuration in 2 yaml files: one for global (portable between computers) and one local (for settings specific to the current installation).

## Limitations

- Uses external codecs installed on your OS (through QtMultimedia, on Linux - GStreamer backend), like the most other multimedia players;
- CUE must contain only 1 source audio file, multi-file CUEs are not supported;
- Lacks some "expected" features like tracks rearranging within playlist.
- Global hotkeys don't work in Wayland

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

#### MacOS

It should be possible to build mpz for Mac, but I don't have a hardware to test it. If you're interested in Mac builds - drop an issue here https://github.com/olegantonyan/mpz/issues/new

## Configuration

The default config location on Linux is `~/.config/mpz`, on Windows - `C:/Users/$USERNAME/AppData/Local/mpz/mpz`. There are 2 files:
- `local.yml` - for the settings specific to this computer, like windows' sizes, playlists, etc;
- `global.yml` - for portable settings that make sense to share between computers.

Some config options can be changed only by editing config files:

- `stream_buffer_size` in `global.yml` - minimal stream buffer size in bytes. The default is 128KB;

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
