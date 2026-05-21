[![GPLv3 License](https://img.shields.io/badge/license-GPL--3.0-blue)](https://github.com/olegantonyan/mpz/blob/master/license.txt)
[![tests](https://github.com/olegantonyan/mpz/actions/workflows/tests.yml/badge.svg)](https://github.com/olegantonyan/mpz/actions/workflows/tests.yml)
[![OmniPackage repositories badge](https://repositories.omnipackage.org/mpz/stable/badge.svg)](https://repositories.omnipackage.org/mpz/stable/install.html)
[![OmniPackage repositories badge](https://repositories.omnipackage.org/mpz/next/badge.svg)](https://repositories.omnipackage.org/mpz/next/install.html)

# Music player for big local collections

![image](https://raw.githubusercontent.com/olegantonyan/mpz/gh-pages/images/mpz-workflow.apng)

If you like organizing your music in folders, then this player might be for you. It doesn't try to index all the files into a library, but rather treats your files and folders as the library and provides a convenient way to create playlists directly from folders. Similar to Foobar2000's Album List, but it's not an attempt to clone.

More screenshots here: https://mpz-player.org

This player is an attempt to create the "perfect" player for the author. It doesn't try to be groundbreaking — it just gets the job done. The main feature is the 3-column UI and the way you manage playlists. Choose library folders, middle-click on a folder, and a playlist will be created from it.

Why "big local collections"? "Local" as opposed to streaming services (which are fine, but this player's goal is playing music you have on your hard drive); "big" means the collection is large enough that managing it becomes hard. Internet radio streaming is also supported.

In version 2.0.0 an experimental [mpd](https://musicpd.org) client mode was added. You can add an mpd server as a library folder. There are limitations and caveats, see below.

## Features

- 3-column UI which allows you to quickly create playlists from folders and switch between playlists;
- Built with C++/Qt - fast and responsive native UI;
- Supports internet radio in `m3u` and `pls` playlists formats;
- Supports CUE sheets;
- Supports MPRIS on Linux for remote control (for example, via [KDE Connect](https://kdeconnect.kde.org/));
- Configuration in 2 yaml files: one for global (portable between computers) and one local (for settings specific to the current installation);
- [mpd](https://musicpd.org) client mode support (version 2.0.0+);
- Tags editor.

## Installation

#### openSUSE, Debian, Fedora, Ubuntu, RedHat, Mageia

**New users:** install from the omnipackage repositories — [stable releases](https://repositories.omnipackage.org/mpz/stable/install.html) (recommended) or [unstable builds aka "next"](https://repositories.omnipackage.org/mpz/next/install.html). These are the primary repositories going forward.

<details>
<summary>Already using the old Open Build Service repositories?</summary>

The previous [Open Build Service repositories](https://software.opensuse.org//download.html?project=home%3Aoleg_antonyan&package=mpz) still exist and existing installations will keep working, but existing users are also encouraged to switch — future packaging effort is focused on omnipackage. The new repositories are signed with different GPG keys, so switching means removing the old repository and adding the new one (the install pages linked above walk through this).

Note: on Debian-based distros (Debian, Ubuntu) the OBS builds were always built against Qt5, even on releases that ship a recent enough Qt6 — making OBS produce working `.deb` packages with Qt6 turned out to be very painful. The omnipackage builds use Qt6 where the distro supports it.

</details>

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

#### macOS (experimental)

Grab the `.dmg` from the releases page: https://github.com/olegantonyan/mpz/releases/. Universal binary, runs on Apple Silicon and Intel Macs (macOS 11 Big Sur or later).

The build is not signed with an Apple Developer ID, so macOS Gatekeeper blocks it on first launch. After dragging `mpz.app` to `/Applications`, remove the quarantine flag from a terminal:

```
xattr -dr com.apple.quarantine /Applications/mpz.app
```

The app will then launch normally.

#### From sources

Dependencies: gcc, make, cmake, qt development headers (libqt5-qtbase-devel, libqt5-qtmultimedia-devel, libqt5-qtx11extras-devel for Qt5 and qt6-base-common-devel, qt6-multimedia-devel, qt6-widgets-devel, qt6-concurrent-devel for Qt6 on openSUSE).
Packages' names may differ in different distros.

```
cmake -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# now you can use build/mpz binary directly
# optionally, install to /usr/local:
sudo cmake --install build
```

To build Qt5 version add `-DUSE_QT5=ON` to cmake cli.

You can also link against shared libraries Taglib, yaml-cpp, or libmpdclient installed on your OS instead of using vendored statically compiled versions. To do this add `-DUSE_SYSTEM_TAGLIB=ON -DUSE_SYSTEM_YAMLCPP=ON -DUSE_SYSTEM_LIBMPDCLIENT=ON` to cmake cli.

## Configuration

The default config location on Linux is `~/.config/mpz`, on Windows - `C:/Users/$USERNAME/AppData/Local/mpz/mpz`. There are 2 files:
- `local.yml` - for the settings specific to this computer, like windows' sizes, playlists, etc;
- `global.yml` - for portable settings that make sense to share between computers.

Starting from version 2.0.8 there is settings dialog where all these options can be changed via GUI.

- `inhibit_sleep_while_playing` in `global.yml` - when `true` player will prevent your OS from automatic sleep while playing (on Linux requires `systemd-inhibit`);
- `stream_buffer_size` in `global.yml` - minimal stream buffer size in bytes. The default is 128KB;
- `single_instance` in `global.yml` - when `true` the player will reuse 1 instance, launching another instance with files as command line arguments will send these files to running instance as a new playlist;
- `single_instance_ipc_port` in `global.yml` - single instance functionality uses TCP socket, this option allows you to specify a port;
- `playback_log_size` in `global.yml` - max size of playback log, default is 100;
- `columns_config` in `global.yml` - configure columns in playlist section, more on this below;
- `playlist_row_height` in `global.yml` - sets playlist's row height in pixels, by default it comes from your desktop theme, but in KDE Plasma 5.27 this height was increased for no apparent reason, can be useful in other DEs;
- `stop_when_track_removed` in `global.yml` - when `true` removing the currently playing track (or the playlist that contains it) stops playback and clears the playlist view;

If you messed up any of the config options you can remove it completely (or even remove the whole file) and it will reset to default.

#### Columns config

You can change the default columns in the playlist view via `columns_config` option in `global.yml` file. The defaults are:

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

Available alignments: left, right.

The sum of `width_percent` of all columns must add up to 100 or below. Sometimes it has to be below 100 to get rid of horizontal scroll; this may happen due to padding and a few extra pixels in your desktop theme.

`stretch` will stretch the column to fit the window width to the right. It's advised to have the last column stretched and the sum of all `width_percent` below 100, but you can experiment with it and see how it looks on your desktop.

#### Lyrics

The track info dialog (right-click a track → "Track info") shows lyrics next to the metadata. Three providers are available, tried in order until one returns lyrics:

1. `embedded` - lyrics stored in tags (ID3v2 USLT, Vorbis Comment LYRICS, MP4 ©lyr, APE LYRICS);
2. `sidecar` - a `<filename>.lrc` or `<filename>.txt` file next to the audio file. LRC timestamps are stripped for plain-text rendering;
3. `lrclib` - online lookup via [LRCLIB](https://lrclib.net) (open, no API key required).

The default order is `[embedded, sidecar, lrclib]`. To override (for example, to disable online lookup, or change the order), add a `lyrics:` block to `global.yml`:

```
lyrics:
  providers: [embedded, sidecar]
```

#### Block certain MPRIS senders

You can ignore MPRIS commands from certain senders, for example, in `global.yml` file:
```
mpris_blacklist: ["wireplumber"]
```

This will ignore commands issued by Wireplumber. Starting with version around 0.5 it has a feature that cannot be disabled - whenever the audio device disconnects it issues an MPRIS Pause command. Until they make it configurable, blocking wireplumber is a viable workaround if you also find this feature annoying.

## Limitations

- Uses external codecs installed on your OS (through QtMultimedia, using ffmpeg or GStreamer backend on Linux);
- Global hotkeys don't work in Wayland.

Starting at Qt 6.4, QtMultimedia supports ffmpeg backend on Linux. You can enable it via environment variable QT_MEDIA_BACKEND: `QT_MEDIA_BACKEND=ffmpeg mpz`.

NOTE: currently on openSUSE Tumbleweed (~ year 2024) they seem to be using ffmpeg by default and this may cause issues. You can switch to gstreamer via the same environment variable `QT_MEDIA_BACKEND=gstreamer mpz`.

### mpd impedance mismatch
When used as an [mpd](https://musicpd.org) client, there is a fundamental difference that can lead to some weird behavior. By design mpz does not have an explicit playback queue - the playlist itself is the queue. In mpd, there's an explicit playback queue and playlists are merely lists of tracks that can be loaded into the queue to play.

Known issues:
- when another client modifies the playback queue, mpz cannot pick up these changes;
- upon start, if mpd is already playing a song, mpz can recognize it only if this song is from the last selected playlist, i.e. the one loaded at startup;
- "playback follows cursor" cannot follow into a different playlist;

## [Changelog](https://github.com/olegantonyan/mpz/blob/master/CHANGELOG.md)
