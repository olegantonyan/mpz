#include "replaygain/manager.h"

#include "config/global.h"
#include "config/storage.h"
#include "playlist/cueparser.h"
#include "playlist/loader.h"
#include "replaygain/tags.h"

#include <QDir>
#include <QDirIterator>
#include <QStandardPaths>
#include <QHash>
#include <QSet>
#include <QTimer>

namespace ReplayGain {
  namespace {
    const char kModeTrack[] = "track";
    const char kModeAlbum[] = "album";
    const char kStorageTags[] = "tags";
    const char kStorageSidecar[] = "sidecar";

    // album gain holds a ~1 MB ebur128 state per file until the folder is done
    const int kMaxAlbumFiles = 64;

    QString appliedName(Applied applied) {
      switch (applied) {
        case Applied::Track: return QStringLiteral("track");
        case Applied::Album: return QStringLiteral("album");
        case Applied::Fallback: break;
      }
      return QStringLiteral("fallback");
    }

    QString sourceName(Source source) {
      switch (source) {
        case Source::Sidecar: return QStringLiteral("sidecar");
        case Source::Tags: return QStringLiteral("tags");
        case Source::Cue: return QStringLiteral("cue");
        case Source::None: break;
      }
      return QStringLiteral("none");
    }

    bool isAudioFile(const QFileInfo &entry) {
      const QString suffix = entry.suffix().toLower();
      return suffix != QLatin1String("cue") &&
             Playlist::Loader::supportedFileFormats().contains(suffix);
    }

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
    const QString override = qEnvironmentVariable("MPZ_CONFIG_DIR_OVERRIDE");
    if (!override.isEmpty()) {
      return override;
    }
    // a library-sized database is data, not config
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
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
      store_.endBatch();
      store_.pruneMissing(walked_folders);
      walked_folders.clear();
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

  QString Manager::appliedGainText(const Track &track) {
    if (settings_.mode == Mode::Off || track.isMpd() ||
        (!track.isStream() && track.path().isEmpty())) {
      return QString();
    }

    const Resolved resolved = resolver_.resolve(track);
    return QStringLiteral("ReplayGain: %1 %2 dB (%3)")
        .arg(appliedName(appliedKind(resolved.gain, settings_)),
             QString::number(effectiveGainDb(resolved.gain, settings_), 'f', 2),
             sourceName(resolved.source));
  }

  QString Manager::statusText(const Track &track) {
    if (settings_.mode == Mode::Off) {
      return QStringLiteral("ReplayGain: off");
    }
    const QString applied = appliedGainText(track);
    return applied.isEmpty() ? QStringLiteral("ReplayGain: %1").arg(tokenFromMode(settings_.mode))
                             : applied;
  }

  void Manager::onSliceAnalyzed(const ReplayGain::SliceResult &result) {
    if (result.ok) {
      store_.put(result.path, result.begin_ms, result.gain);
      resolver_.invalidate(result.path, result.begin_ms);
      dirty = true;
      scheduleGainsChanged();
    }
    emit sliceAnalyzed(result);
  }

  void Manager::scheduleGainsChanged() {
    if (gains_changed_scheduled) {
      return;
    }
    gains_changed_scheduled = true;
    QTimer::singleShot(0, this, [this]() {
      gains_changed_scheduled = false;
      emit gainsChanged();
    });
  }

  void Manager::cancelScan() {
    walking = false;
    walk_folders.clear();
    scanner.cancel();
  }

  void Manager::setScanWorker(const QString &program, const QStringList &arguments) {
    scanner.setWorker(program, arguments);
  }

  void Manager::scanTracks(const QVector<Track> &tracks, bool force) {
    walking = false;
    walk_folders.clear();
    walked_folders.clear();
    store_.beginBatch();
    progress_done = 0;
    progress_total = 0;
    progress_path.clear();
    listing_folder.clear();

    scanner.start(planScan(tracks, force));
  }

  void Manager::scanLibrary(const QStringList &roots, bool force) {
    if (isScanning()) {
      return;
    }

    listing_folder.clear();
    walk_folders.clear();
    walked_folders.clear();
    store_.beginBatch();
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

    const QVector<ScanTarget> targets = targetsInFolder(folder);
    if (!targets.isEmpty()) {
      // only folders holding audio, so pruning walks album directories and not the
      // whole subtree under every parent
      walked_folders << folder;
      scanner.enqueue(planTargets(targets, walk_force));
    }
    QTimer::singleShot(0, this, &Manager::walkNextFolder);
  }

  const QFileInfoList &Manager::folderFiles(const QString &folder) const {
    const QString normalized = QDir(folder).absolutePath();
    if (normalized != listing_folder) {
      listing_folder = normalized;
      listing_files = QDir(normalized).entryInfoList(QDir::Files);
    }
    return listing_files;
  }

  QVector<ScanTarget> Manager::targetsInFolder(const QString &folder) const {
    const QFileInfoList entries = folderFiles(folder);

    QVector<ScanTarget> targets;
    QSet<QString> in_cue;
    for (const auto &entry : entries) {
      if (entry.suffix().toLower() != QLatin1String("cue")) {
        continue;
      }
      for (const auto &track : Playlist::CueParser(entry.absoluteFilePath()).tracks_list()) {
        in_cue.insert(QFileInfo(track.path()).canonicalFilePath());
        targets.append({track.path(), QFileInfo(track.path()), track.begin(), track.duration()});
      }
    }

    for (const auto &entry : entries) {
      if (!isAudioFile(entry)) {
        continue;
      }
      // canonicalFilePath() is a realpath() call; only a cue can alias a file
      if (!in_cue.isEmpty() && in_cue.contains(entry.canonicalFilePath())) {
        continue;
      }
      targets.append({entry.absoluteFilePath(), entry, 0, 0});
    }
    return targets;
  }

  QVector<Job> Manager::planScan(const QVector<Track> &tracks, bool force) const {
    QVector<ScanTarget> targets;
    targets.reserve(tracks.size());
    for (const auto &track : tracks) {
      if (track.isStream() || track.isMpd() || track.path().isEmpty()) {
        continue;
      }
      const QFileInfo info(track.path());
      if (!isAudioFile(info)) {
        continue;
      }
      targets.append({track.path(), info, track.begin(),
                      track.isCue() ? track.duration() : 0});
    }
    return planTargets(targets, force);
  }

  QVector<Job> Manager::planTargets(const QVector<ScanTarget> &targets, bool force) const {
    QVector<QString> order;
    QHash<QString, Job> by_folder;
    QHash<QString, int> file_index;
    FileStats stats;

    for (const auto &target : targets) {
      if (!target.info.exists()) {
        continue;
      }
      stats.insert(target.path, target.info);

      const QString folder = target.info.absolutePath();
      if (!by_folder.contains(folder)) {
        Job job;
        job.folder = folder;
        job.write_tags = settings_.storage == StorageMode::Tags;
        by_folder.insert(folder, job);
        order.append(folder);
      }

      Slice slice;
      slice.begin_ms = target.begin_ms;
      slice.duration_ms = target.duration_ms;

      Job &job = by_folder[folder];
      const auto known = file_index.constFind(target.path);
      int index = known == file_index.constEnd() ? -1 : *known;
      if (index < 0) {
        FileWork work;
        work.path = target.path;
        job.files.append(work);
        index = job.files.size() - 1;
        file_index.insert(target.path, index);
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
      job.want_album = job.files.size() <= kMaxAlbumFiles && coversWholeAlbum(job, stats);

      if (!force) {
        if (job.want_album) {
          if (albumAlreadyAnalysed(job, stats)) {
            continue;
          }
        } else {
          dropAnalysedSlices(job, stats);
          if (job.files.isEmpty()) {
            continue;
          }
        }
      }
      jobs.append(job);
    }
    return jobs;
  }

  bool Manager::coversWholeAlbum(const Job &job, const FileStats &stats) const {
    QSet<QString> scanned;
    for (const auto &work : job.files) {
      if (!coversWholeFile(work)) {
        return false;
      }
      scanned.insert(stats.value(work.path).absoluteFilePath());
    }

    for (const auto &entry : folderFiles(job.folder)) {
      if (isAudioFile(entry) && !scanned.contains(entry.absoluteFilePath())) {
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

  Gain Manager::analysedGain(const QString &path, const QFileInfo &info, quint64 begin_ms,
                             bool sliced) const {
    // in tags mode the tags are the storage; a sliced file only fits the sidecar
    if (settings_.storage == StorageMode::Tags && !sliced) {
      return readTags(path);
    }
    return store_.get(path, info, begin_ms);
  }

  bool Manager::albumAlreadyAnalysed(const Job &job, const FileStats &stats) const {
    for (const auto &work : job.files) {
      const QFileInfo info = stats.value(work.path);
      const bool sliced = work.slices.size() > 1;
      for (const auto &slice : work.slices) {
        const Gain existing = analysedGain(work.path, info, slice.begin_ms, sliced);
        if (!existing.has_track || !existing.has_album) {
          return false;
        }
      }
    }
    return true;
  }

  void Manager::dropAnalysedSlices(Job &job, const FileStats &stats) const {
    for (auto &work : job.files) {
      const QFileInfo info = stats.value(work.path);
      const bool sliced = work.slices.size() > 1;
      const auto analysed = [this, &work, &info, sliced](const Slice &slice) {
        return analysedGain(work.path, info, slice.begin_ms, sliced).has_track;
      };
      work.slices.erase(std::remove_if(work.slices.begin(), work.slices.end(), analysed),
                        work.slices.end());
    }
    const auto empty = [](const FileWork &work) { return work.slices.isEmpty(); };
    job.files.erase(std::remove_if(job.files.begin(), job.files.end(), empty), job.files.end());
  }
}
