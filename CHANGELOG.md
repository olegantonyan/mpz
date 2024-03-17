## [1.0.24] - 
### Added

### Fixed
- start position of track with CUE

## [1.0.23] - 2023-02-18
### Added
- ability to inhibit sleep on Windows
- "Open config directory" main menu option
- workaround for KDE Plasma 5.27 - playlist row height can be set in global config via "playlist_row_height"

## [1.0.22] - 2022-11-07
### Added
- track info window: context menu for cover art
- Serbian language translation
- scroll playlists view to bottom upon addind new playlist

### Misc
- update built-in Taglib to 1.13

## [1.0.21] - 2022-08-28
### Misc
- new endpoint for feedback form (Heroku no longer free)

## [1.0.20] - 2022-05-28
### Added
- keyboard shortcut to jump to playing track
- config files location in "about" dialog

### Fixed
- reduce stdout spam from sleep lock

### Misc
- ability to build against system taglib and yaml-cpp (used by default for AUR packages)

## [1.0.19] - 2021-12-08

### Fixed
- saving renamed playlist

### Added
- support embedded album covers
- show covers in track info dialog

## [1.0.18] - 2021-11-28

### Fixed
- build with older Qt versions fixed
- MPRIS track info updates with the stream metadata

## [1.0.17] - 2021-11-28
### Added
- expose album covers through MPRIS: show them in KDE media player widget

### Fixed
- MPRIS fix seek (was broken in KDE media player widget)

## [1.0.16] - 2021-11-24
### Added
- Ability to inhibit automatic sleep while playing on Linux

### Misc
- Qt6 support
- Update QHotKey to 1.5.0

## [1.0.15] - 2021-08-24
### Added
- "Save settings" main menu item to force saving settings and playlists to both config files
- Ability to select playlist via enter key
- Added "Search on web" track dialog menu item

### Misc
- Update yaml cpp to 0.7.0

## [1.0.14] - 2021-08-16
### Added
- "Play" context menu item in playlists view
- "Jump to current track" context menu item status bar
- "Keyboard shortcuts" menu item
- Ability to sort library tree by date

### Misc
- Update Taglib to 1.12
- Update QHotKey to 1.4.2

## [1.0.13] - 2021-03-13
### Added
- Sort by directory
- Stream keep alive

## [1.0.12] - 2021-03-13
### Skipped

## [1.0.11] - 2020-12-13
### Added
- Configurable playlist columns
- Playback log size config option

### Fixed
- Show in file manager works on stream playlists
- Single instance IPC port change to non-ephemeral (31341)

## [1.0.10] - 2020-11-07
### Added
- Option to reload playlist from filesystem
- Verbose title string in playback log
- Double click on playlist starts playing the first track

## [1.0.9] - 2020-11-01
### Added
- Option to keep only one instance

## [1.0.8] - 2020-10-31
### Added
- Support CUE with multiple files

## [1.0.7] - 2020-10-22
### Added
- Create new plylist from command line arguments
- MimeTypes support - able to open audio files in filemanager
- Icon of diffrent sizes for better desktop integration

## [1.0.6] - 2020-10-21
### Fixed
- Tracks rows height set to minimum regardless of distro default

### Added
- Context menu icons

## [1.0.5] - 2020-10-20
### Added
- Load and save m3u playlists

## [1.0.4] - 2020-10-19
### Added
- Playlist custom sort with UI

## [1.0.3] - 2020-10-16
### Added
- Ability to create playlist from multiple folders (multiselect)

## [1.0.2] - 2020-10-12
### Added
- Minimize to tray option
- Support opus files
- Double click on file in library creates a new playlist containing this file
- "Delete" key works in playlists and tracks view
- Added missing dependency on Debian and Ubuntu

## [1.0.1] - 2020-10-08
### Hotfixes
- Fix crash upon append to current playlist when there is no playlists
- Fixed typo in Russian translation

## [1.0.0] - 2020-10-08
### Initial public release
