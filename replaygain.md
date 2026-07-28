# ReplayGain in mpz

Implementation notes. Feature is complete and tested; this is context for future work on it.

## Layout

```
3rdparty/libebur128-1.2.6/     upstream release archive, unmodified
app/replaygain/
  gain.h        Gain struct, Settings, LUFS/R128 conversions, effectiveGainDb()   header-only
  store.{h,cpp} append-only TSV sidecar + in-memory QHash
  analyzer.{h,cpp}  libebur128 wrapper (one per track or cue slice)
  tags.{h,cpp}  read/write REPLAYGAIN_* via TagLib
  scanjob.h     Job / Slice / FileWork / SliceResult / JobResult
  jobrunner.{h,cpp}  worker: decodes and measures one folder      Qt6 only
  scanner.{h,cpp}    GUI-side orchestrator, threads + epochs      Qt6 only
  resolver.{h,cpp}   Track -> dB, with cache
  manager.{h,cpp}    settings + store + resolver + scanner + scan planning   Qt6 only
app/replaygain_ui/replaygaindialog.{h,cpp}                        Qt6 only
```

Tests: `tst_replaygain{gain,store,analyzer,tags,scanner,manager}`, plus new cases in
`tst_equalizer` and `tst_cueparser_fixtures`. 49 targets total, all passing.

## Reference levels

ReplayGain 2.0 = **-18 LUFS**. `gain_db = -18 - integrated_lufs`.

Opus is the exception: `R128_TRACK_GAIN` / `R128_ALBUM_GAIN` are **Q7.8 integers** (dB * 256)
against **-23 LUFS**, so converting costs a +5 dB shift (`dbFromR128`, `r128FromDb` in `gain.h`).
The OpusHead `output_gain` is applied by the decoder before we see samples, so `-23 - measured`
is already the correct additional gain — do not add an `output_gain` term.

## Sidecar format

`Config::Storage::configPath() + "/replaygain.db"`, append-only:

```
#mpz-rg 1
-5.57	0.910858	-2.48	0.977203	1718300000	41235012	0	/music/Noel/01.flac
```

`track_gain, track_peak, album_gain, album_peak, mtime, size, begin_ms, path`.
Empty gain field = absent. Path is escaped (`\\`, `\t`, `\n`, `\r`), so a line is always
exactly 8 fields. Key is `path@begin_ms` — the same composite `loader.cpp:117` uses for cue
dedup. `Track::uid()` is random per construction and unusable as a key.

Load = one linear pass into `QHash`, last line wins. A wrong field count drops that line only,
which is why a torn tail from a crash costs one record and not the file. Compacted through
`QSaveFile` when duplicates exceed 30%, on close.

Staleness = stored mtime + size vs `QFileInfo`. Mismatch reads as unscanned.

## Tag quirks (all discovered the hard way)

| Container | Path | Reason |
|---|---|---|
| FLAC | `xiphComment(true)` direct | `FileRef::tag()` is a union containing ID3v1, which reports every `REPLAYGAIN_*` key unsupported — the generic write **fails** |
| Opus | `R128_*_GAIN` Q7.8 | `REPLAYGAIN_*_GAIN` is actively removed so nothing double-corrects. Peaks ride along as `REPLAYGAIN_*_PEAK`, an extension |
| MP3 | `TXXX`, lowercase description | TagLib writes uppercase; every other tagger writes lowercase |
| MP4 | `----:com.apple.iTunes:replaygain_track_gain` | `nameForPropertyKey` (`mp4itemfactory.cpp:264-271`) only ever emits the **uppercase** atom |
| APE / WMA / WAV | generic `PropertyMap` | works as-is |

Reading is case-insensitive everywhere; Opus `R128_*` wins over any stray `REPLAYGAIN_*_GAIN`.
Cue containers never get tags written (one tag set, many tracks) — reported as `Unsupported`.

## Scanning

Unit of work is a **folder**, because album gain is `ebur128_loudness_global_multiple()` over
live states and those cannot cross threads. Album gain is gated loudness over the concatenation,
never the average of track gains.

`QAudioDecoder` needs an event loop, so each file decodes inside a nested `QEventLoop` on a
worker `QThread`. A queued `cancel()` reaches the runner through that nested loop and quits it,
so cancellation works mid-file. 30 s stall watchdog covers a silent decoder.

Workers = `qBound(1, idealThreadCount() - 1, 4)`. Leaving a core free is a correctness
constraint, not politeness: the engine pumps its sink from a 50 ms timer on the GUI thread.

Cancellation uses an epoch counter — `Scanner::cancel()` bumps it, every in-flight `JobResult`
is stale on arrival, and runners finish at their own pace instead of being joined.

## Playback

Chain: `Resolver` -> `Engine::setReplayGainResolver` -> resolved once per segment ->
`Timeline::Segment::gain_db` -> `eq.setExtraGainDb()` in `feedSink`.

**Resolution happens at segment creation, not in `feedSink`.** There are seven
`timeline.reset`/`appendSegment` sites in `engine.cpp` (lines 90, 117, 223, 517, 611, 658, 667).
Resolving can open the file with TagLib, and `feedSink` runs on the GUI thread — I/O there
stalls the sink and the UI together.

Use `pos.segment`, never `Engine::current_track`: the latter follows the audible clock
(`checkSegmentBoundary`) and lags the read cursor by up to the 500 ms sink buffer, which would
apply the previous track's gain to the first half-second of every gapless transition.

### Three Equalizer constraints

1. `setExtraGainDb` must **not** call `rebuild()` — that clears `channel_state_`
   (`equalizer.cpp:97`) and would wipe biquad history on every buffer.
2. `isIdentity()` keeps its old meaning (filters only). The gate is the new `isPassthrough()`,
   so the gain cannot hide behind it.
3. `coeffs_` is filled regardless of `enabled_`, and `process*` ran the filters unconditionally
   once the gate opened. ReplayGain opens that gate, so a **disabled** equalizer would have
   started colouring the sound. `run_eq = !isIdentity()` is hoisted out of the loop.

Gain folds into the same multiply as the EQ preamp, so the int16 path still quantizes and
dithers once. Float path clamps only when `extra_gain_lin_ > 1.0`, keeping EQ-only output
bit-identical to before.

## Config keys (`global.yml`)

| Key | Absent means |
|---|---|
| `replay_gain_mode` | off |
| `replay_gain_storage` | sidecar |
| `replay_gain_preamp_db` | 0 (stored as string — `Config::Value` has no double) |
| `replay_gain_fallback_db` | 0 |
| `replay_gain_allow_clipping` | inverted, so absent = clip prevention on |

## Precedence

Tag mode: file tags only. Sidecar mode: sidecar -> `Track::replayGain()` (cue REM) -> file tags.
Cue tracks skip the file-tag fallback entirely.

`ReplayGain::Gain` on `Track` is runtime-only and **not serialized**. Only `CueParser` fills it,
from `REM REPLAYGAIN_*`, because those values cannot be recovered from the container. Persisting
them in the playlist YAML would create a second source of truth that survives a rescan.

## Gotchas

- `moc` misparses `std::function<double(const Track &)>` as returning `const double`. Hence the
  `Playback::ReplayGainResolver` alias in `mediaplayer.h`.
- `ebur128_state` is an anonymous-struct typedef, so it cannot be forward-declared;
  `analyzer.h` includes `<ebur128.h>`.
- libebur128 declares `cmake_minimum_required(2.8.12)` and attaches no include dirs to its
  target. Both handled from the parent `CMakeLists.txt` (`CMAKE_POLICY_VERSION_MINIMUM`,
  `target_include_directories`) — nothing inside the vendored archive is edited.

## Known gaps

- **Qt5 unverified.** Only Qt 6.11.1 is installed here. Scanner/manager/dialog sit in the Qt6
  block and `ENABLE_GAPLESS` is only defined there, so Qt5 should be unaffected — untested.
  `store`, `analyzer`, `tags`, `resolver` compile unconditionally and are Qt5-safe by inspection.
- **`Track::reload()` clobbers cue metadata** (`track.cpp:156`): no `isCue()` guard, so it
  replaces cue-derived title/artist and resets `_duration` to the container length. Pre-existing,
  reachable from `playlistcontroller.cpp:214,232,245`. Fix is `return _cue ? false : readMetadata();`.
  Left alone as out of scope. A corrupted duration would produce wrong cue slices on a later scan.
- **Filename encoding is inconsistent** across existing TagLib call sites: `track.cpp:109,127`
  use `toUtf8()`, `tageditordialog.cpp:33` uses `QFile::encodeName`. On Windows the `const char*`
  overload reads the local codepage, so a non-ASCII path can open in one and fail in the other.
  `tags.cpp` uses `encodeName` on POSIX and the `wchar_t` overload on Windows.
- **`last_filtered_frame` drift** (`engine.cpp:890` vs `:896`): on a partial sink write the two
  diverge and the next `feedSink` calls a spurious `eq.reset()`. Pre-existing and unchanged by
  this feature — with ReplayGain on and the EQ off, `channel_state_` is empty so `reset()` is a
  no-op.
- No UI for pruning entries whose files are gone. `Store::compact()` keeps them.

## Build

```
cmake -S . -B build -DBUILD_TESTING=ON && cmake --build build && ctest --test-dir build
```

Local toolchain: `/Users/oleg/Qt/Tools/CMake/CMake.app/Contents/bin/cmake`,
`/Users/oleg/Qt/Tools/Ninja/ninja`, Qt at `/Users/oleg/Qt/6.11.1/macos`.

Test fixtures `tests/fixtures/replaygain/*` are four ffmpeg-generated 0.2 s silent files
(~13 KB total). The scanner test synthesizes its own WAVs instead.
