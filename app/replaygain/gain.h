#ifndef REPLAYGAIN_GAIN_H
#define REPLAYGAIN_GAIN_H

#include <algorithm>
#include <cmath>

namespace ReplayGain {
  struct Gain {
    double track_db = 0.0;
    double track_peak = 0.0;
    double album_db = 0.0;
    double album_peak = 0.0;
    bool has_track = false;
    bool has_album = false;

    bool isValid() const { return has_track || has_album; }
  };

  constexpr double kReferenceLufs = -18.0;
  constexpr double kR128ReferenceLufs = -23.0;

  inline double gainDbFromLufs(double lufs) {
    return kReferenceLufs - lufs;
  }

  // R128 references -23 LUFS, ReplayGain -18.
  inline double dbFromR128(int q7_8) {
    return q7_8 / 256.0 + (kReferenceLufs - kR128ReferenceLufs);
  }

  inline int r128FromDb(double db) {
    const long v = std::lround((db - (kReferenceLufs - kR128ReferenceLufs)) * 256.0);
    return static_cast<int>(std::clamp(v, -32768L, 32767L));
  }

  inline int r128FromLufs(double lufs) {
    return r128FromDb(gainDbFromLufs(lufs));
  }

  enum class Mode { Off, Track, Album };
  enum class StorageMode { Sidecar, Tags };

  struct Settings {
    Mode mode = Mode::Off;
    StorageMode storage = StorageMode::Sidecar;
    double preamp_db = 0.0;
    double fallback_db = 0.0;
    bool prevent_clipping = true;
  };

  constexpr double kMinGainDb = -30.0;
  constexpr double kMaxGainDb = 12.0;

  inline double clampGainDb(double db) {
    return std::clamp(db, kMinGainDb, kMaxGainDb);
  }

  inline double effectiveGainDb(const Gain &g, const Settings &s) {
    if (s.mode == Mode::Off) {
      return 0.0;
    }

    const bool album_first = s.mode == Mode::Album;
    double db = 0.0;
    double peak = 0.0;
    bool have = false;

    if (album_first && g.has_album) {
      db = g.album_db;
      peak = g.album_peak;
      have = true;
    } else if (!album_first && g.has_track) {
      db = g.track_db;
      peak = g.track_peak;
      have = true;
    } else if (g.has_track) {
      db = g.track_db;
      peak = g.track_peak;
      have = true;
    } else if (g.has_album) {
      db = g.album_db;
      peak = g.album_peak;
      have = true;
    }

    if (!have) {
      return clampGainDb(s.fallback_db);
    }

    db += s.preamp_db;
    if (s.prevent_clipping && peak > 0.0) {
      db = std::min(db, -20.0 * std::log10(peak));
    }
    return clampGainDb(db);
  }
}

#endif // REPLAYGAIN_GAIN_H
