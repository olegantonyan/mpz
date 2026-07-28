#include "replaygain/manager.h"

#include "config/global.h"
#include "config/storage.h"

#include <QFileInfo>
#include <QHash>

namespace ReplayGain {
  namespace {
    const char kModeTrack[] = "track";
    const char kModeAlbum[] = "album";
    const char kStorageTags[] = "tags";
    const char kStorageSidecar[] = "sidecar";

    Mode modeFromToken(const QString &token) {
      if (token == QLatin1String(kModeTrack)) {
        return Mode::Track;
      }
      if (token == QLatin1String(kModeAlbum)) {
        return Mode::Album;
      }
      return Mode::Off;
    }

    QString tokenFromMode(Mode mode) {
      switch (mode) {
        case Mode::Track: return kModeTrack;
        case Mode::Album: return kModeAlbum;
        case Mode::Off: break;
      }
      return QString();
    }
  }

  QString Manager::storeDirectory() {
    return Config::Storage::configPath();
  }

  Manager::Manager(Config::Global &global_c, QObject *parent) :
    QObject(parent), global_conf(global_c), store_(storeDirectory()), resolver_(&store_) {
    load();

    connect(&scanner, &Scanner::sliceAnalyzed, this, &Manager::onSliceAnalyzed);
    connect(&scanner, &Scanner::progress, this, &Manager::progress);
    connect(&scanner, &Scanner::finished, this, [this](int analysed, int failed, bool cancelled) {
      if (dirty) {
        dirty = false;
        resolver_.invalidate();
        emit gainsChanged();
      }
      emit scanFinished(analysed, failed, cancelled);
    });
  }

  void Manager::load() {
    settings_.mode = modeFromToken(global_conf.replayGainMode());
    settings_.storage = global_conf.replayGainStorage() == QLatin1String(kStorageTags)
                            ? StorageMode::Tags
                            : StorageMode::Sidecar;
    settings_.preamp_db = global_conf.replayGainPreampDb();
    settings_.fallback_db = global_conf.replayGainFallbackDb();
    settings_.prevent_clipping = !global_conf.replayGainAllowClipping();
    resolver_.setSettings(settings_);
  }

  void Manager::persist() {
    global_conf.saveReplayGainMode(tokenFromMode(settings_.mode));
    global_conf.saveReplayGainStorage(settings_.storage == StorageMode::Tags ? kStorageTags
                                                                            : kStorageSidecar);
    global_conf.saveReplayGainPreampDb(settings_.preamp_db);
    global_conf.saveReplayGainFallbackDb(settings_.fallback_db);
    global_conf.saveReplayGainAllowClipping(!settings_.prevent_clipping);
  }

  void Manager::setSettings(const Settings &s) {
    settings_ = s;
    resolver_.setSettings(settings_);
    persist();
    emit settingsChanged();
    emit gainsChanged();
  }

  void Manager::onSliceAnalyzed(const ReplayGain::SliceResult &result) {
    if (result.ok) {
      store_.put(result.path, result.begin_ms, result.gain);
      dirty = true;
    }
    emit sliceAnalyzed(result);
  }

  void Manager::cancelScan() {
    scanner.cancel();
  }

  void Manager::scanTracks(const QVector<Track> &tracks, Mode scan_mode, bool force) {
    scanner.start(planScan(tracks, scan_mode, force));
  }

  QVector<Job> Manager::planScan(const QVector<Track> &tracks, Mode scan_mode, bool force) const {
    const bool want_album = scan_mode == Mode::Album;

    QVector<QString> order;
    QHash<QString, Job> by_folder;

    for (const auto &track : tracks) {
      if (track.isStream() || track.isMpd() || track.path().isEmpty()) {
        continue;
      }
      const QFileInfo info(track.path());
      if (!info.exists()) {
        continue;
      }
      if (!force) {
        const Gain existing = store_.get(track.path(), track.begin());
        if (existing.has_track && (!want_album || existing.has_album)) {
          continue;
        }
      }

      const QString folder = info.absolutePath();
      if (!by_folder.contains(folder)) {
        Job job;
        job.folder = folder;
        job.want_album = want_album;
        job.write_tags = settings_.storage == StorageMode::Tags;
        by_folder.insert(folder, job);
        order.append(folder);
      }

      Slice slice;
      slice.begin_ms = track.begin();
      slice.duration_ms = track.isCue() ? track.duration() : 0;

      Job &job = by_folder[folder];
      int index = -1;
      for (int i = 0; i < job.files.size(); i++) {
        if (job.files.at(i).path == track.path()) {
          index = i;
          break;
        }
      }
      if (index < 0) {
        FileWork work;
        work.path = track.path();
        job.files.append(work);
        index = job.files.size() - 1;
      }
      job.files[index].slices.append(slice);
    }

    QVector<Job> jobs;
    jobs.reserve(order.size());
    for (const auto &folder : order) {
      Job job = by_folder.value(folder);
      for (auto &work : job.files) {
        std::sort(work.slices.begin(), work.slices.end(),
                  [](const Slice &a, const Slice &b) { return a.begin_ms < b.begin_ms; });
      }
      jobs.append(job);
    }
    return jobs;
  }
}
