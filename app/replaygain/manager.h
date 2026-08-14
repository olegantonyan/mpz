#ifndef REPLAYGAIN_MANAGER_H
#define REPLAYGAIN_MANAGER_H

#include "replaygain/resolver.h"
#include "replaygain/scanner.h"
#include "replaygain/store.h"
#include "track.h"
#include "config/global.h"

#include <QFileInfo>
#include <QFileInfoList>
#include <QHash>
#include <QObject>
#include <QStringList>
#include <QVector>

namespace ReplayGain {
  // path is what the store keys by; info carries the stat taken while listing
  struct ScanTarget {
    QString path;
    QFileInfo info;
    quint64 begin_ms = 0;
    quint64 duration_ms = 0;
  };

  class Manager : public QObject {
    Q_OBJECT
  public:
    explicit Manager(Config::Global &global_conf, QObject *parent = nullptr);

    Settings settings() const { return settings_; }
    void setSettings(const Settings &s);

    Store &store() { return store_; }
    Resolver &resolver() { return resolver_; }

    double gainDbFor(const Track &track) { return resolver_.gainDbFor(track); }

    // untranslated "ReplayGain: album -7.25 dB (sidecar)", empty when none applies
    QString appliedGainText(const Track &track);
    // the same, falling back to the bare mode so the status label always has something
    QString statusText(const Track &track);

    bool isScanning() const { return scanner.isScanning(); }
    int progressDone() const { return progress_done; }
    int progressTotal() const { return progress_total; }
    QString progressPath() const { return progress_path; }
    QVector<Job> planScan(const QVector<Track> &tracks, bool force) const;
    void setScanWorker(const QString &program, const QStringList &arguments);
    void scanTracks(const QVector<Track> &tracks, bool force);
    void scanLibrary(const QStringList &roots, bool force);
    void cancelScan();

    static QString storeDirectory();

  signals:
    void settingsChanged();
    void gainsChanged();
    void progress(int done, int total, const QString &path);
    void sliceAnalyzed(const ReplayGain::SliceResult &result);
    void scanFinished(int analysed, int failed, bool cancelled);

  private:
    using FileStats = QHash<QString, QFileInfo>;

    void load();
    void persist();
    void onSliceAnalyzed(const ReplayGain::SliceResult &result);
    void scheduleGainsChanged();
    QVector<Job> planTargets(const QVector<ScanTarget> &targets, bool force) const;
    Gain analysedGain(const QString &path, const QFileInfo &info, quint64 begin_ms, bool sliced) const;
    bool coversWholeAlbum(const Job &job, const FileStats &stats) const;
    static bool coversWholeFile(const FileWork &work);
    bool albumAlreadyAnalysed(const Job &job, const FileStats &stats) const;
    void dropAnalysedSlices(Job &job, const FileStats &stats) const;
    void walkNextFolder();
    QVector<ScanTarget> targetsInFolder(const QString &folder) const;
    const QFileInfoList &folderFiles(const QString &folder) const;

    Config::Global &global_conf;
    Store store_;
    Resolver resolver_;
    Scanner scanner;
    Settings settings_;
    bool dirty = false;
    bool gains_changed_scheduled = false;
    int progress_done = 0;
    int progress_total = 0;
    QString progress_path;
    QStringList walk_folders;
    QStringList walked_folders;
    bool walk_force = false;
    bool walking = false;
    mutable QString listing_folder;
    mutable QFileInfoList listing_files;
  };
}

#endif // REPLAYGAIN_MANAGER_H
