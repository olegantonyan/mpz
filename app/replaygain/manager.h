#ifndef REPLAYGAIN_MANAGER_H
#define REPLAYGAIN_MANAGER_H

#include "replaygain/resolver.h"
#include "replaygain/scanner.h"
#include "replaygain/store.h"
#include "track.h"
#include "config/global.h"

#include <QObject>
#include <QStringList>
#include <QVector>

namespace ReplayGain {
  class Manager : public QObject {
    Q_OBJECT
  public:
    explicit Manager(Config::Global &global_conf, QObject *parent = nullptr);

    Settings settings() const { return settings_; }
    void setSettings(const Settings &s);

    Store &store() { return store_; }
    Resolver &resolver() { return resolver_; }

    double gainDbFor(const Track &track) { return resolver_.gainDbFor(track); }

    bool isScanning() const { return scanner.isScanning(); }
    int progressDone() const { return progress_done; }
    int progressTotal() const { return progress_total; }
    QString progressPath() const { return progress_path; }
    QVector<Job> planScan(const QVector<Track> &tracks, bool force) const;
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
    void load();
    void persist();
    void onSliceAnalyzed(const ReplayGain::SliceResult &result);
    bool coversWholeAlbum(const Job &job) const;
    static bool coversWholeFile(const FileWork &work);
    bool albumAlreadyAnalysed(const Job &job) const;
    void dropAnalysedSlices(Job &job) const;
    void walkNextFolder();
    static QVector<Track> tracksInFolder(const QString &folder);

    Config::Global &global_conf;
    Store store_;
    Resolver resolver_;
    Scanner scanner;
    Settings settings_;
    bool dirty = false;
    int progress_done = 0;
    int progress_total = 0;
    QString progress_path;
    QStringList walk_folders;
    bool walk_force = false;
    bool walking = false;
  };
}

#endif // REPLAYGAIN_MANAGER_H
