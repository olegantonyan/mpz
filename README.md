[![GPLv3 License](https://img.shields.io/badge/license-GPL--3.0-blue)](https://github.com/olegantonyan/mpz/blob/master/license.txt)
[![tests](https://github.com/olegantonyan/mpz/actions/workflows/tests.yml/badge.svg)](https://github.com/olegantonyan/mpz/actions/workflows/tests.yml)
[![OmniPackage repositories badge x86_64](https://repositories.omnipackage.org/mpz/stable/badge.svg)](https://repositories.omnipackage.org/mpz/stable/install.html)
[![OmniPackage repositories badge aarch64](https://repositories.omnipackage.org/mpz/stable-aarch64/badge.svg)](https://repositories.omnipackage.org/mpz/stable-aarch64/install.html)
[![Flathub](https://img.shields.io/flathub/v/org.mpz_player.mpz?logo=flathub&label=flathub)](https://flathub.org/apps/org.mpz_player.mpz)

# Folder player for big local music collections

![image](https://raw.githubusercontent.com/olegantonyan/mpz/gh-pages/images/mpz-workflow.apng)

If you like organizing your music in folders, then this player might be for you. It doesn't try to index all the files into a library, but rather treats your files and folders as the library and provides a convenient way to create playlists directly from folders. Similar to Foobar2000's Album List, but not an attempt to clone it.

More screenshots here: https://mpz-player.org

This player is an attempt to create the "perfect" player for the author. It doesn't try to be groundbreaking — it just gets the job done. The main feature is the 3-column UI and the way you manage playlists. Choose library folders, middle-click on a folder, and a playlist will be created from it.

Why "big local collections"? "Local" as opposed to streaming services (which are fine, but this player's goal is playing music you have on your hard drive); "big" means the collection is large enough that managing it becomes hard. Internet radio streaming is also supported.

In version 2.0.0 an experimental [mpd](https://musicpd.org) client mode was added. You can add an mpd server as a library folder. There are limitations and caveats, see below.

## Features

- 3-column UI to quickly create playlists from folders and switch between them
- Native C++/Qt UI - fast and responsive
- Drag-n-drop files and folders from file manager
- Gapless playback
- Equalizer with 10-band and full parametric modes, supports presets from AutoEq, SquigLink and others
- Internet radio in `m3u` and `pls` formats, as well as built-in radio library
- CUE sheets, with gapless playback of single-file albums
- Tag editor
- Dynamic range meter: per-track and album DR log, in the foobar2000 Dynamic Range Meter format
- Cover art and lyrics with opt-in online sources
- Playback order per playlist and global: sequential, random, or no-loop
- Track sorting presets
- Global media-key hotkeys and a built-in keyboard shortcuts dialog
- Media/OS integration: MPRIS on Linux (remote control, e.g. via [KDE Connect](https://kdeconnect.kde.org/)), SMTC and taskbar controls on Windows, Now Playing and native menu/Dock on macOS, system tray / macOS menu bar
- Update check on Windows, macOS, and Linux AppImage
- UI languages: English, Russian, Japanese, Serbian (see [adding a translation](#adding-a-translation))
- Configuration in 2 yaml files: global (portable between computers) and local (specific to this installation)
- [mpd](https://musicpd.org) client mode
- ReplayGain that can write either tags or separate file

## Supported formats

- Audio: mp3, flac, ogg/oga, opus, spx, m4a/m4b/mp4, aac, wav, aiff/aif/aifc, wma, asf, ape, wv, mpc, tta, mka, dsf, dff, shn, and CUE sheets
- Playlists and radio: m3u, m3u8, pls

Decoding uses your OS codecs, so exact format support depends on what is installed.

## Installation

#### Linux

openSUSE, Debian, Fedora, Ubuntu, RedHat, Mageia, Arch, Manjaro native packages:

- **Stable** (recommended): [x86_64](https://repositories.omnipackage.org/mpz/stable/install.html) | [aarch64](https://repositories.omnipackage.org/mpz/stable-aarch64/install.html)
- **Next** (unstable builds from master): [x86_64](https://repositories.omnipackage.org/mpz/next/install.html) | [aarch64](https://repositories.omnipackage.org/mpz/next-aarch64/install.html)

Arch and Manjaro are x86_64 only. These repos contain only one package - mpz - and bring no risk of breaking anything else on your system, you can also download package directly without adding repository.

<details>
<summary>Already using Open Build Service repositories?</summary>

[Open Build Service repositories](https://software.opensuse.org//download.html?project=home%3Aoleg_antonyan&package=mpz) still exist, but existing users are encouraged to switch - future packaging effort is focused on omnipackage. The new repositories are signed with different GPG keys, so switching means removing the old repository and adding the new one (the install pages linked above walk through this).

</details>

##### AUR

An Arch AUR package is also available: https://aur.archlinux.org/packages/mpz/

```
git clone https://aur.archlinux.org/mpz.git
cd mpz
makepkg -si
```

##### AppImage

AppImage builds (x86_64 and aarch64) are on the [releases page](https://github.com/olegantonyan/mpz/releases/latest). Prefer the native repositories above if your distro is supported.

##### Flatpak

Available on Flathub: https://flathub.org/apps/org.mpz_player.mpz. Prefer the native repositories above if your distro is supported.

**NOTE: limited filesystem access** - Flatpak apps are sandboxed. Paths within `$HOME`, `/mnt`, `/media`, `/run/media` have full unsandboxed access. Paths outside of them can still be added as library folders, but won't refresh automatically when files are added while mpz is running - a limitation of desktop portals.

#### Windows

Grab the installer or portable binary from the [releases page](https://github.com/olegantonyan/mpz/releases/latest). Qt6 builds require Windows 10+. Legacy Qt5 can probably run on earlier versions, only use it if you run old 32-bit OS below Windows 10, this build has issues playing radio streams, lacks some features, and provided only as a fallback option for very old systems.

* win-x86_64-qt6 - x86 64-bit build (recommended)
* win-arm64-qt6 - native ARM 64-bit build (only ARM CPU)
* win-legacy-qt5 - legacy version for x86 32-bit systems (only Windows below 10)

<details>
<summary>The build is not signed, so Windows blocks it on first launch.</summary>

**Microsoft Defender SmartScreen** - "Windows protected your PC". Click *More info*, then *Run anyway*. No *Run anyway*? Right-click the file → *Properties* → tick *Unblock* → *Apply*.

**Smart App Control** - "cannot be verified", with nothing to click; it has no exception list. Turn it off in *Windows Security → App & browser control → Smart App Control*, install, turn it back on. Older Windows 11 builds made that switch one-way, so check you can re-enable it first. Only clean installs of Windows 11 have it on.

To uninstall, use the "Uninstall mpz" Start Menu shortcut or Control Panel. Settings → Apps may not work on Windows 11 ([a known Qt Installer Framework bug](https://bugreports.qt.io/projects/QTIFW/issues/QTIFW-3336)).

</details>

#### macOS

Grab the `.dmg` from the [releases page](https://github.com/olegantonyan/mpz/releases/latest). Universal binary, runs on Apple Silicon and Intel Macs (macOS 11 Big Sur or later).

<details>
<summary>The build is not signed with an Apple Developer ID, so macOS Gatekeeper blocks it on first launch.</summary>

After dragging `mpz music player.app` to `/Applications`, use either method below.

**Terminal:** remove the quarantine flag, then open the app normally:

```
xattr -dr com.apple.quarantine "/Applications/mpz music player.app"
```

**System Settings:** try to open `mpz music player.app` once and dismiss the warning, then go to *System Settings → Privacy & Security*. Near the bottom you'll see a message that mpz was blocked — click *Open Anyway* and confirm.

The app will then launch normally.

</details>

#### From sources

Dependencies: gcc, make, cmake, Qt development headers (Multimedia, Widgets, Concurrent, Svg)

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# now you can use build/mpz binary directly
# optionally, install to /usr/local:
sudo cmake --install build
```

You can also link against shared libraries Taglib, yaml-cpp, libmpdclient, QHotkey, libebur128, sqlite, or cpptrace installed on your OS instead of using vendored statically compiled versions. To do this add `-DUSE_SYSTEM_TAGLIB=ON -DUSE_SYSTEM_YAMLCPP=ON -DUSE_SYSTEM_LIBMPDCLIENT=ON -DUSE_SYSTEM_QHOTKEY=ON -DUSE_SYSTEM_CPPTRACE=ON -DUSE_SYSTEM_LIBEBUR128=ON -DUSE_SYSTEM_SQLITE3=ON` to cmake cli.

Other options: 
- `-DUSE_QT5=ON` builds Qt5 legacy version
- `-DENABLE_DBUS=OFF` drops Linux MPRIS support
- `-DENABLE_MPD_SUPPORT=OFF` drops mpd client mode
- `-DENABLE_QHOTKEY=OFF` drops global media-key hotkeys (on by default, except macOS and Windows MSVC where the OS owns media keys) 
- `-DENABLE_CRASH_HANDLER=OFF` drops the builtin crash handler (on by default on Linux, macOS and Windows MSVC - the only platforms cpptrace builds on)
- `-DENABLE_GAPLESS=OFF` drops the gapless playback engine (on by default, Qt6 only).

## Qt6/Qt5 split

Some features require Qt6:

- Gapless playback
- Equalizer
- Audio output switch
- Dynamic range meter
- ReplayGain

This means win-legacy-qt5 won't have these. As well as old Linux distros. Qt5 support will be dropped at some point in future.

## Configuration

Starting from version 2.0.8 there is a settings dialog where all these options can be changed via the GUI.

<details>
<summary>Manual yaml configuration</summary>

The default config location on Linux is `~/.config/mpz`, on Windows - `C:/Users/$USERNAME/AppData/Local/mpz/mpz`. There are 2 files:
- `local.yml` - for the settings specific to this computer, like windows' sizes, playlists, etc
- `global.yml` - for portable settings that make sense to share between computers

The available config options are:

- `inhibit_sleep_while_playing` in `global.yml` - when `true` the player will prevent your OS from sleeping automatically while playing (on Linux requires `systemd-inhibit`)
- `stream_buffer_size` in `global.yml` - minimum stream buffer size in bytes. The default is 128KB
- `single_instance` in `global.yml` - when `true` the player will reuse a single instance — launching another instance with files as command-line arguments will send these files to the running instance as a new playlist
- `playback_log_size` in `global.yml` - max size of playback log, default is 100
- `columns_config` in `global.yml` - configure columns in the playlist section, more on this below
- `show_playlist_headers` in `global.yml` - when `true` the playlist shows column headers. Default is `false`
- `playlist_row_height` in `global.yml` - sets playlist's row height in pixels, by default it comes from your desktop theme, but in KDE Plasma 5.27 this height was increased for no apparent reason, can be useful in other DEs
- `stop_when_track_removed` in `global.yml` - when `true` removing the currently playing track (or the playlist that contains it) stops playback and clears the playlist view
- `disable_qhotkey` in `local.yml` - when `true` mpz does not grab the global media keys. Only X11 and legacy Windows builds grab them at all on Wayland grabbing is impossible, and elsewhere the OS integration delivers media keys instead
- `shortcuts` in `global.yml` - rebound keyboard shortcuts, more on this below

If you messed up any of the config options you can remove it completely (or even remove the whole file) and it will reset to default.

#### Shortcuts config

Usually edited in the app, but writable by hand:

```
shortcuts:
  next: Ctrl+Alt+N
  open_sort_menu: ''
```

Only changed shortcuts are stored; the rest follow the platform defaults, so `global.yml` stays portable. Empty string means unbound. Unparseable values are ignored. On a collision the rebound action wins and the other is unset.

Sequences use portable spelling (`Ctrl`, `Alt`, `Shift`, `Meta`). On macOS `Ctrl+` is ⌘, `Meta+` is ⌃ and `Alt+` is ⌥.

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

Available fields: artist, album_artist, album, title, year, length, path, url, sample_rate, bitrate, channels, track_number, disc_number, format, filename.

Available alignments: left, right.

The sum of `width_percent` of all columns must add up to 100 or less. Sometimes it has to be below 100 to get rid of horizontal scroll; this may happen due to padding and a few extra pixels in your desktop theme.

`stretch` will stretch the column to fit the window width to the right. It's advised to have the last column stretched and the sum of all `width_percent` below 100, but you can experiment with it and see how it looks on your desktop.

#### Lyrics

The track info dialog (right-click a track → "Track info") shows lyrics next to the metadata. Providers are tried in order until one returns lyrics:

1. `embedded` - lyrics stored in tags (ID3v2 USLT, Vorbis Comment LYRICS, MP4 ©lyr, APE LYRICS)
2. `sidecar` - a `<filename>.lrc` or `<filename>.txt` file next to the audio file. LRC timestamps are stripped for plain-text rendering
3. `lrclib` - online lookup via [LRCLIB](https://lrclib.net) (open, no API key required)

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

All of them are editable in that dialog: click a key field, press the new combination, OK. Global media keys are not rebindable.

## Limitations

- Uses external codecs installed on your OS (through QtMultimedia, using ffmpeg or GStreamer backend on Linux)
- Global hotkeys don't work in Wayland (https://github.com/olegantonyan/mpz/issues/129, but in KDE Plasma, for example, global media keys work and send MPRIS commands to the player)

Starting at Qt 6.4, QtMultimedia supports ffmpeg backend on Linux, and it seems like they are heading towards enabling it by default. You can set the multimedia backend via the environment variable QT_MEDIA_BACKEND: `QT_MEDIA_BACKEND=ffmpeg mpz` or `QT_MEDIA_BACKEND=gstreamer mpz`.

### mpd impedance mismatch
When used as an [mpd](https://musicpd.org) client, there is a fundamental difference that can lead to some weird behavior. By design mpz does not have an explicit playback queue - the playlist itself is the queue. In mpd, there's an explicit playback queue and playlists are merely lists of tracks that can be loaded into the queue to play.

Known issues:
- when another client modifies the playback queue, mpz cannot pick up these changes
- upon start, if mpd is already playing a song, mpz can recognize it only if this song is from the last selected playlist, i.e. the one loaded at startup
- "playback follows cursor" cannot follow into a different playlist

## Contributing

### Adding a translation

Translations live in `app/resources/translations/` as `.ts` (source) and `.qm` (compiled) files, embedded via `app/resources.qrc`. Run the commands from the repo root.

1. Create or update the source file (scans the `app` sources):

   ```
   lupdate app -ts app/resources/translations/<lang>.ts
   ```
2. Translate the strings in Qt Linguist
3. Compile it to `.qm` (written next to the `.ts`):

   ```
   lrelease app/resources/translations/<lang>.ts
   ```
4. Add `<lang>.qm` to `app/resources.qrc`

### Running tests

`cmake --workflow tests-qt6` (or `tests-qt5`) configures, builds, and runs the unit tests.

`cmake --workflow gui-tests-qt6` runs the widget tests. They need a session bus and an audio sink.

`cmake --workflow mpd-tests-qt6` runs the MPD end-to-end tests. They need `mpd` installed; each test binary
starts its own server on a free port with a throwaway config, so a running system mpd is left alone.

## [Changelog](https://github.com/olegantonyan/mpz/blob/master/CHANGELOG.md)
