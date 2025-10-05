[![GPLv3 License](https://img.shields.io/badge/License-GPL%20v3-yellow.svg)](https://opensource.org/licenses/)
[![build result](https://build.opensuse.org/projects/home:oleg_antonyan/packages/mpz/badge.svg?type=percent)](https://build.opensuse.org/package/show/home:oleg_antonyan/mpz)
[![OmniPackage repositories badge](https://repositories.omnipackage.org/oleg/mpz/mpz.svg)](https://web.omnipackage.org/oleg/mpz)

# Music player for big local collections

![image](https://raw.githubusercontent.com/olegantonyan/mpz/gh-pages/images/mpz-workflow.apng)

If you like organizing your music in folders, then this player might be for you. It doesn't try to index all the files into a library, but rather treats your files and folders as the library and provides a convenient way to create playlists directly from folders. Similar to Foobar2000's Album List, but it's not an attempt to clone.

More screenshots here: https://mpz-player.org

## Why?

In about 15 years author couldn't find a suitable player for Linux. Foobar2000 works in Wine, but this solution is not perfect either. This player is an attempt to create the "perfect" player for the author. It doesn't have anything "breakthrough", it just gets the job done. The main feature is 3-columns UI and the way you manage playlists. Chose library folders, middle-click on a folder and a playlist will be created from this folder.

Why "big local collections"? "Local" opposed to streaming services (which are fine, but this player's goal is playing music you have on your hard drive), "big" means it's big enough so managing it becomes hard. Radio streaming also supported.

## Features

- 3-columns UI which allows you to quickly create playlists from folders and switch between playlists;
- Built with C++/Qt - fast and responsive native UI;
- Supports internet radio in `m3u` and `pls` playlists formats;
- Supports CUE sheets;
- Supports MPRIS on Linux for remote control (for example, via [KDE Connect](https://kdeconnect.kde.org/));
- Configuration in 2 yaml files: one for global (portable between computers) and one local (for settings specific to the current installation).

## Limitations

- Uses external codecs installed on your OS (through QtMultimedia, using ffmpeg or GStreamer backend on Linux);
- Lacks some "expected" features like tracks rearranging within playlist;
- Global hotkeys don't work in Wayland.

Starting at Qt 6.4, QtMultimedia supports ffmpeg backend on Linux. You can enable it via environment variable QT_MEDIA_BACKEND: `QT_MEDIA_BACKEND=ffmpeg mpz`.

NOTE: currently on openSUSE Tumbleweed (~ year 2024) they seem to be using ffmpeg by default and this may cause issues. You can switch to gstreamer via the same environment variable `QT_MEDIA_BACKEND=gstreamer mpz`.

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

For Qt5 version use this package: https://aur.archlinux.org/packages/mpz-qt5

```
git clone https://aur.archlinux.org/mpz-qt5.git
cd mpz-qt5
makepkg -si
```

#### Windows

Grab installer or portable binary from releases page: https://github.com/olegantonyan/mpz/releases/.

#### From sources

Dependencies: gcc, make, cmake, qt development headers (libqt5-qtbase-devel, libqt5-qtmultimedia-devel, libqt5-qtx11extras-devel for Qt5 and qt6-base-common-devel, qt6-multimedia-devel, qt6-widgets-devel, qt6-concurrent-devel for Qt6 on openSUSE).
Packages' names may differ in different distros.

```
git clone git@github.com:olegantonyan/mpz.git
cd mpz
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release .. # for Qt5 add -DUSE_QT5
make -j`nproc`
# now you now use mpz binary directly
# optionally, install to /usr/local:
sudo make install
```

You can also link against shared libraries Taglib and/or Yaml-cpp installed on your OS instead of using vendored statically compiled versions.
To do this you have to add `-DUSE_SYSTEM_TAGLIB=ON -DUSE_SYSTEM_YAMLCPP=ON` to cmake cli.

```
git clone git@github.com:olegantonyan/mpz.git
cd mpz
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DUSE_SYSTEM_TAGLIB=ON -DUSE_SYSTEM_YAMLCPP=ON ..
make -j`nproc`
# now you now use mpz binary directly
# optionally, install to /usr/local:
sudo make install
```


## Configuration

The default config location on Linux is `~/.config/mpz`, on Windows - `C:/Users/$USERNAME/AppData/Local/mpz/mpz`. There are 2 files:
- `local.yml` - for the settings specific to this computer, like windows' sizes, playlists, etc;
- `global.yml` - for portable settings that make sense to share between computers.

Some config options can be changed only by editing config files:

- `inhibit_sleep_while_playing` in `global.yml` - when `true` player will prevent your OS from automatic sleep while playing (on Linux requires `systemd-inhibit`);
- `stream_buffer_size` in `global.yml` - minimal stream buffer size in bytes. The default is 128KB;
- `single_instance` in `global.yml` - when `true` the player will reuse 1 instance, launching another instance with files as command line arguments will send these files to running instance as a new playlist;
- `single_instance_ipc_port` in `global.yml` - single instance functionality uses TCP socket, this option allows you to specify a port;
- `playback_log_size` in `global.yml` - max size of playback log, default is 100;
- `columns_config` in `global.yml` - configure columns in playlist section, more on this below;
- `playlist_row_height` in `global.yml` - sets playlist's row height in pixels, by default it comes from your desktop theme, but in KDE Plasma 5.27 this height was increased for no apparent reason, can be useful in other DEs;

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

#### Block certain MPRIS senders

You can ignore MPRIS commands from certain senders, for example, in `global.yml` file:
```
mpris_blacklist: ["wireplumber"]
```

This will ignore commands issued by Wireplumber. Starting with version around 0.5 it has a feature that cannot be disabled - whenever the audio device disconnects it issues MPRIS Pause comamnd. Until they make it configurable, blocking wireplumber is viable workaround if you also find this feature annoying.


## [Changelog](https://github.com/olegantonyan/mpz/blob/master/CHANGELOG.md)
