#include "replaygain/manager.h"

#include "config/global.h"
#include "config/storage.h"
#include "playlist/cueparser.h"
#include "playlist/loader.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QHash>
#include <QSet>
#include <QTimer>

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
    connect(&scanner, &Scanner::progress, this,
            [this](int done, int total, const QString &path) {
              progress_done = done;
              progress_total = total;
              if (!path.isEmpty()) {
                progress_path = path;
              }
              emit progress(done, total, path);
            });
    connect(&scanner, &Scanner::finished, this, [this](int analysed, int failed, bool cancelled) {
      if (dirty) {
        dirty = false;
        store_.compactIfNeeded();
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
    walking = false;
    walk_folders.clear();
    scanner.cancel();
  }

  void Manager::scanTracks(const QVector<Track> &tracks, bool force) {
    scanner.start(planScan(tracks, force));
  }

  void Manager::scanLibrary(const QStringList &roots, bool force) {
    if (isScanning()) {
      return;
    }

    walk_folders.clear();
    for (const auto &root : roots) {
      if (!root.startsWith(QLatin1String("mpd://"))) {
        walk_folders << root;
      }
    }
    walk_force = force;
    walking = true;
    progress_done = 0;
    progress_total = 0;
    progress_path.clear();

    scanner.open();
    QTimer::singleShot(0, this, &Manager::walkNextFolder);
  }

  void Manager::walkNextFolder() {
    if (!walking) {
      return;
    }
    if (walk_folders.isEmpty()) {
      walking = false;
      scanner.producerFinished();
      return;
    }

    const QString folder = walk_folders.takeFirst();
    QDirIterator subdirs(folder, QDir::Dirs | QDir::NoDotAndDotDot);
    while (subdirs.hasNext()) {
      walk_folders << subdirs.next();
    }

    const QVector<Track> tracks = tracksInFolder(folder);
    if (!tracks.isEmpty()) {
      scanner.enqueue(planScan(tracks, walk_force));
    }
    QTimer::singleShot(0, this, &Manager::walkNextFolder);
  }

  QVector<Track> Manager::tracksInFolder(const QString &folder) {
    const QFileInfoList entries = QDir(folder).entryInfoList(QDir::Files);

    QVector<Track> tracks;
    QSet<QString> in_cue;
    for (const auto &entry : entries) {
      if (entry.suffix().toLower() != QLatin1String("cue")) {
        continue;
      }
      for (const auto &track : Playlist::CueParser(entry.absoluteFilePath()).tracks_list()) {
        in_cue.insert(QFileInfo(track.path()).canonicalFilePath());
        tracks << track;
      }
    }

    for (const auto &entry : entries) {
      const QString suffix = entry.suffix().toLower();
      if (suffix == QLatin1String("cue") ||
          !Playlist::Loader::supportedFileFormats().contains(suffix) ||
          in_cue.contains(entry.canonicalFilePath())) {
        continue;
      }
      tracks << Track(entry.absoluteFilePath());
    }
    return tracks;
  }

  QVector<Job> Manager::planScan(const QVector<Track> &tracks, bool force) const {
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

      const QString folder = info.absolutePath();
      if (!by_folder.contains(folder)) {
        Job job;
        job.folder = folder;
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
      job.want_album = coversWholeAlbum(job);

      if (!force) {
        if (job.want_album) {
          if (albumAlreadyAnalysed(job)) {
            continue;
          }
        } else {
          dropAnalysedSlices(job);
          if (job.files.isEmpty()) {
            continue;
          }
        }
      }
      jobs.append(job);
    }
    return jobs;
  }

  bool Manager::coversWholeAlbum(const Job &job) const {
    QSet<QString> scanned;
    for (const auto &work : job.files) {
      if (!coversWholeFile(work)) {
        return false;
      }
      scanned.insert(QFileInfo(work.path).absoluteFilePath());
    }

    const auto entries = QDir(job.folder).entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const auto &entry : entries) {
      const QString suffix = entry.suffix().toLower();
      if (suffix == QLatin1String("cue") ||
          !Playlist::Loader::supportedFileFormats().contains(suffix)) {
        continue;
      }
      if (!scanned.contains(entry.absoluteFilePath())) {
        return false;
      }
    }
    return true;
  }

  bool Manager::coversWholeFile(const FileWork &work) {
    quint64 next = 0;
    for (const auto &slice : work.slices) {
      if (slice.begin_ms != next) {
        return false;
      }
      next = slice.duration_ms > 0 ? slice.begin_ms + slice.duration_ms : next;
    }
    return true;
  }

  bool Manager::albumAlreadyAnalysed(const Job &job) const {
    for (const auto &work : job.files) {
      for (const auto &slice : work.slices) {
        const Gain existing = store_.get(work.path, slice.begin_ms);
        if (!existing.has_track || !existing.has_album) {
          return false;
        }
      }
    }
    return true;
  }

  void Manager::dropAnalysedSlices(Job &job) const {
    for (auto &work : job.files) {
      const auto analysed = [this, &work](const Slice &slice) {
        return store_.get(work.path, slice.begin_ms).has_track;
      };
      work.slices.erase(std::remove_if(work.slices.begin(), work.slices.end(), analysed),
                        work.slices.end());
    }
    const auto empty = [](const FileWork &work) { return work.slices.isEmpty(); };
    job.files.erase(std::remove_if(job.files.begin(), job.files.end(), empty), job.files.end());
  }
}
