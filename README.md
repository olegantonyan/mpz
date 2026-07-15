[![GPLv3 License](https://img.shields.io/badge/license-GPL--3.0-blue)](https://github.com/olegantonyan/mpz/blob/master/license.txt)
[![tests](https://github.com/olegantonyan/mpz/actions/workflows/tests.yml/badge.svg)](https://github.com/olegantonyan/mpz/actions/workflows/tests.yml)
[![OmniPackage repositories badge](https://repositories.omnipackage.org/mpz/stable/badge.svg)](https://repositories.omnipackage.org/mpz/stable/install.html)
[![OmniPackage repositories badge](https://repositories.omnipackage.org/mpz/next/badge.svg)](https://repositories.omnipackage.org/mpz/next/install.html)

# Folder player for big local music collections

![image](https://raw.githubusercontent.com/olegantonyan/mpz/gh-pages/images/mpz-workflow.apng)

If you like organizing your music in folders, then this player might be for you. It doesn't try to index all the files into a library, but rather treats your files and folders as the library and provides a convenient way to create playlists directly from folders. Similar to Foobar2000's Album List, but not an attempt to clone it.

More screenshots here: https://mpz-player.org

This player is an attempt to create the "perfect" player for the author. It doesn't try to be groundbreaking — it just gets the job done. The main feature is the 3-column UI and the way you manage playlists. Choose library folders, middle-click on a folder, and a playlist will be created from it.

Why "big local collections"? "Local" as opposed to streaming services (which are fine, but this player's goal is playing music you have on your hard drive); "big" means the collection is large enough that managing it becomes hard. Internet radio streaming is also supported.

In version 2.0.0 an experimental [mpd](https://musicpd.org) client mode was added. You can add an mpd server as a library folder. There are limitations and caveats, see below.

## Features

- 3-column UI to quickly create playlists from folders and switch between them;
- Native C++/Qt UI - fast and responsive;
- Drag-n-drop files and folders from file manager;
- Internet radio in `m3u` and `pls` formats;
- CUE sheets, with seamless playback of single-file albums;
- Tag editor;
- Cover art and lyrics in the track info dialog or as dockable panels that follow the playing track;
- Playback order per playlist and global: sequential, random, or no-loop;
- Track sorting presets;
- Global media-key hotkeys and a built-in keyboard shortcuts dialog;
- Media/OS integration: MPRIS on Linux (remote control, e.g. via [KDE Connect](https://kdeconnect.kde.org/)), SMTC and taskbar controls on Windows, Now Playing and native menu/Dock on macOS, system tray / macOS menu bar;
- Update check on Windows, macOS, and Linux AppImage;
- UI languages: English, Russian, Japanese, Serbian (see [adding a translation](#adding-a-translation));
- Configuration in 2 yaml files: global (portable between computers) and local (specific to this installation);
- [mpd](https://musicpd.org) client mode (version 2.0.0+).

## Supported formats

- Audio: mp3, flac, ogg, opus, m4a/mp4, aac, wav, wma, ape, dsf, and CUE sheets;
- Playlists and radio: m3u, m3u8, pls.

Decoding uses your OS codecs (see [Limitations](#limitations)), so exact format support depends on what is installed.

## Installation

#### openSUSE, Debian, Fedora, Ubuntu, RedHat, Mageia, Arch, Manjaro

Install from the omnipackage repositories (x86_64 arch):

* [stable releases](https://repositories.omnipackage.org/mpz/stable/install.html) (recommended)
* [unstable aka "next"](https://repositories.omnipackage.org/mpz/next/install.html)

<details>
<summary>Already using the old Open Build Service repositories?</summary>

The previous [Open Build Service repositories](https://software.opensuse.org//download.html?project=home%3Aoleg_antonyan&package=mpz) still exist and existing installations will keep working, but existing users are also encouraged to switch — future packaging effort is focused on omnipackage. The new repositories are signed with different GPG keys, so switching means removing the old repository and adding the new one (the install pages linked above walk through this).

Note: on Debian-based distros (Debian, Ubuntu) the OBS builds were always built against Qt5, even on releases that ship a recent enough Qt6 — making OBS produce working `.deb` packages with Qt6 turned out to be very painful. The omnipackage builds use Qt6 where the distro supports it.

</details>

##### AUR

An Arch AUR package is also available if you prefer it over binary repositories: https://aur.archlinux.org/packages/mpz/

```
git clone https://aur.archlinux.org/mpz.git
cd mpz
makepkg -si
```

For Qt5 version use this package: https://aur.archlinux.org/packages/mpz-qt5

##### AppImage

Experimental AppImage builds (x86_64 and arm64) are on the [releases page](https://github.com/olegantonyan/mpz/releases/latest). Prefer the native repositories above if your distro is supported.

#### Windows

Grab the installer or portable binary from the [releases page](https://github.com/olegantonyan/mpz/releases/latest). Qt6 builds require Windows 10+. Qt5 can probably run on earlier versions.

* win-x86_64 - x86 64-bit build (recommended for most users)
* win-arm64 - native ARM 64-bit build
* win-legacy - legacy version for x86 32-bit systems

win-x86_64 build should also work on ARM64 systems thanks to Windows' emulation layer, but the performance may suffer. Use win-arm64 if you have an ARM64 Windows PC.

To uninstall, use the "Uninstall mpz" Start Menu shortcut or Control Panel. Settings → Apps may not work on Windows 11 ([a known Qt Installer Framework bug](https://bugreports.qt.io/projects/QTIFW/issues/QTIFW-3336)).

#### macOS

Grab the `.dmg` from the [releases page](https://github.com/olegantonyan/mpz/releases/latest). Universal binary, runs on Apple Silicon and Intel Macs (macOS 11 Big Sur or later).

The build is not signed with an Apple Developer ID, so macOS Gatekeeper blocks it on first launch. After dragging `mpz music player.app` to `/Applications`, use either method below.

**Terminal:** remove the quarantine flag, then open the app normally:

```
xattr -dr com.apple.quarantine "/Applications/mpz music player.app"
```

**System Settings:** try to open `mpz music player.app` once and dismiss the warning, then go to *System Settings → Privacy & Security*. Near the bottom you'll see a message that mpz was blocked — click *Open Anyway* and confirm.

The app will then launch normally.

#### From sources

Dependencies: gcc, make, cmake, qt development headers (libqt5-qtbase-devel, libqt5-qtmultimedia-devel, libqt5-qtx11extras-devel, libqt5-qtsvg-devel for Qt5 and qt6-base-common-devel, qt6-multimedia-devel, qt6-widgets-devel, qt6-concurrent-devel, qt6-svg-devel for Qt6 on openSUSE).
Package names may differ between distros.

```
cmake -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# now you can use build/mpz binary directly
# optionally, install to /usr/local:
sudo cmake --install build
```

To build Qt5 version add `-DUSE_QT5=ON` to cmake cli.

You can also link against shared libraries Taglib, yaml-cpp, libmpdclient, QHotkey, or cpptrace installed on your OS instead of using vendored statically compiled versions. To do this add `-DUSE_SYSTEM_TAGLIB=ON -DUSE_SYSTEM_YAMLCPP=ON -DUSE_SYSTEM_LIBMPDCLIENT=ON -DUSE_SYSTEM_QHOTKEY=ON -DUSE_SYSTEM_CPPTRACE=ON` to cmake cli.

Other options: `-DENABLE_DBUS=OFF` drops Linux MPRIS support, `-DENABLE_MPD_SUPPORT=OFF` drops mpd client mode, `-DENABLE_QHOTKEY=OFF` drops global media-key hotkeys (on by default, except macOS and Windows MSVC where the OS owns media keys), `-DENABLE_CRASH_HANDLER=OFF` drops the builtin crash handler (on by default on Linux, macOS and Windows MSVC — the only platforms cpptrace builds on).

## Configuration

Starting from version 2.0.8 there is a settings dialog where all these options can be changed via the GUI.

<details>
<summary>Manual yaml configuration</summary>

The default config location on Linux is `~/.config/mpz`, on Windows - `C:/Users/$USERNAME/AppData/Local/mpz/mpz`. There are 2 files:
- `local.yml` - for the settings specific to this computer, like windows' sizes, playlists, etc;
- `global.yml` - for portable settings that make sense to share between computers.

The available config options are:

- `inhibit_sleep_while_playing` in `global.yml` - when `true` the player will prevent your OS from sleeping automatically while playing (on Linux requires `systemd-inhibit`);
- `stream_buffer_size` in `global.yml` - minimum stream buffer size in bytes. The default is 128KB;
- `single_instance` in `global.yml` - when `true` the player will reuse a single instance — launching another instance with files as command-line arguments will send these files to the running instance as a new playlist;
- `single_instance_ipc_port` in `global.yml` - single instance functionality uses TCP socket, this option allows you to specify a port;
- `playback_log_size` in `global.yml` - max size of playback log, default is 100;
- `columns_config` in `global.yml` - configure columns in the playlist section, more on this below;
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

The sum of `width_percent` of all columns must add up to 100 or less. Sometimes it has to be below 100 to get rid of horizontal scroll; this may happen due to padding and a few extra pixels in your desktop theme.

`stretch` will stretch the column to fit the window width to the right. It's advised to have the last column stretched and the sum of all `width_percent` below 100, but you can experiment with it and see how it looks on your desktop.

#### Lyrics

The track info dialog (right-click a track → "Track info") shows lyrics next to the metadata. Providers are tried in order until one returns lyrics:

1. `embedded` - lyrics stored in tags (ID3v2 USLT, Vorbis Comment LYRICS, MP4 ©lyr, APE LYRICS);
2. `sidecar` - a `<filename>.lrc` or `<filename>.txt` file next to the audio file. LRC timestamps are stripped for plain-text rendering;
3. `lrclib` - online lookup via [LRCLIB](https://lrclib.net) (open, no API key required).

Additional online providers, off by default: `netease` (NetEase), `qq` (QQ Music), `lyrics.ovh` (Lyrics.ovh).

The default order is `[embedded, sidecar, lrclib]`. To override (change the order, disable online lookup, or add providers), add a `lyrics:` block to `global.yml`:

```
lyrics:
  providers: [embedded, sidecar]
```

#### Block certain MPRIS senders

You can ignore MPRIS commands from certain senders, for example, in `global.yml` file:
```
mpris_blacklist: ["wireplumber"]
```

This will ignore commands issued by Wireplumber. Starting around version 0.5, it has a feature that cannot be disabled - whenever the audio device disconnects it issues an MPRIS Pause command. Until they make it configurable, blocking wireplumber is a viable workaround if you also find this feature annoying.

</details>

## Keyboard shortcuts

The full, platform-aware list is in the app: press Alt+S (Linux/Windows) or ⌘+/ (macOS), or open the main menu → "Keyboard shortcuts".

Common ones: Space - play/pause; Ctrl+1/2/3 - focus the three panes; Ctrl+L - playback log; Ctrl+J - jump to the playing track.

## Limitations

- Uses external codecs installed on your OS (through QtMultimedia, using ffmpeg or GStreamer backend on Linux);
- Global hotkeys don't work in Wayland (https://github.com/olegantonyan/mpz/issues/129, but in KDE Plasma, for example, global media keys work and send MPRIS commands to the player).

Starting at Qt 6.4, QtMultimedia supports ffmpeg backend on Linux, and it seems like they are heading towards enabling it by default. You can set the multimedia backend via the environment variable QT_MEDIA_BACKEND: `QT_MEDIA_BACKEND=ffmpeg mpz` or `QT_MEDIA_BACKEND=gstreamer mpz`.

### mpd impedance mismatch
When used as an [mpd](https://musicpd.org) client, there is a fundamental difference that can lead to some weird behavior. By design mpz does not have an explicit playback queue - the playlist itself is the queue. In mpd, there's an explicit playback queue and playlists are merely lists of tracks that can be loaded into the queue to play.

Known issues:
- when another client modifies the playback queue, mpz cannot pick up these changes;
- upon start, if mpd is already playing a song, mpz can recognize it only if this song is from the last selected playlist, i.e. the one loaded at startup;
- "playback follows cursor" cannot follow into a different playlist;

## Contributing

### Adding a translation

Translations live in `app/resources/translations/` as `.ts` (source) and `.qm` (compiled) files, embedded via `app/resources.qrc`. Run the commands from the repo root.

1. Create or update the source file (scans the `app` sources):

   ```
   lupdate app -ts app/resources/translations/<lang>.ts
   ```
2. Translate the strings in Qt Linguist;
3. Compile it to `.qm` (written next to the `.ts`):

   ```
   lrelease app/resources/translations/<lang>.ts
   ```
4. Add `<lang>.qm` to `app/resources.qrc`.

### Running tests

`cmake --workflow tests-qt6` (or `tests-qt5`) configures, builds, and runs the unit tests.

## [Changelog](https://github.com/olegantonyan/mpz/blob/master/CHANGELOG.md)
