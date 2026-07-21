#ifndef LOCAL_H
#define LOCAL_H

#include "storage.h"
#include "singleinstanceguard.h"
#include "playlist/playlist.h"

#include <QByteArray>

namespace Config {
  class Local : public SingleInstanceGuard<Local> {
  public:
    explicit Local();

    bool sync();

    bool saveWindowGeometry(const QByteArray &v);
    QByteArray windowGeomentry() const;

    bool saveWindowState(const QByteArray &v);
    QByteArray windowState() const;

    bool saveSplitterSizes(const QList<int> &list);
    QList<int> splitterSizes() const;

    bool toolbarMovable() const;
    bool saveToolbarMovable(bool v);

    QList<std::shared_ptr<Playlist::Playlist>> playlists() const;
    bool savePlaylists(QList<std::shared_ptr<Playlist::Playlist>> &list);

    int currentPlaylist() const;
    bool saveCurrentPlaylist(int idx);

    QMap<QString, QString> currentMpdPlaylist() const;
    bool saveCurrentMpdPlaylist(const QMap<QString, QString> &name_for_library_url);

    QMap<QString, QList<QString>> mpdPlaylistsOrder() const;
    bool saveMpdPlaylistsOrder(const QMap<QString, QList<QString>> &playlists_for_library_url);

    QStringList libraryPaths() const;
    bool saveLibraryPaths(const QStringList &arg);

    int currentLibraryPath() const;
    bool saveCurrentLibraryPath(int arg);

    int libraryViewScrollPosition() const;
    bool saveLibraryViewScrollPosition(int val);

    int playlistViewScrollPosition() const;
    bool savePlaylistViewScrollPosition(int val);

    int volume() const;
    bool saveVolume(int arg);

    int totalPlaybackTime() const;
    bool saveTotalPlaybackTime(int arg);

    QByteArray outputDeviceId() const;
    bool saveOutputDeviceId(const QByteArray &arg);

    QString crashReportConsent() const;
    void saveCrashReportConsent(const QString &arg);

    QString lastReportedCrash() const;
    void saveLastReportedCrash(const QString &arg);

  private:
    Config::Storage storage;

    Config::Value serializeTrack(const Track &t) const;
    Track deserializeTrack(const Config::Value &v) const;

    bool durationSeconds;
    quint64 trackDuration(int value) const;
  };
}

#endif // LOCAL_H
