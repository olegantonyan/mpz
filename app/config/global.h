#ifndef GLOBAL_H
#define GLOBAL_H

#include "storage.h"
#include "sort_ui/sortingpresetsdialog.h"
#include "playlist_ui/columnsconfig.h"

#include <QPair>
#include <QList>

namespace Config {
  class Global {
  public:
    explicit Global();

    bool sync();

    bool playbackFollowCursor() const;
    void savePlaybackFollowCursor(bool arg);

    QString playbackOrder() const;
    void savePlaybackOrder(const QString &arg);

    bool trayIconEnabled() const;
    void saveTrayIconEnabled(bool arg);

    int streamBufferSize() const;
    void saveStreamBufferSize(int arg);

    bool minimizeToTray() const;
    void saveMinimizeToTray(bool arg);

    QList<SortingPreset> sortPresets() const;
    bool saveSortPresets(const QList<SortingPreset> &arg);

    QString language() const;
    void saveLanguage(const QString &arg);

    int ipcPort() const;
    bool saveIpcPort(int arg);

    bool singleInstance() const;
    void saveSingleInstance(bool arg);

    int playbackLogSize() const;
    void savePlaybackLogSize(int arg);

    PlaylistUi::ColumnsConfig columnsConfig() const;
    bool saveColumnsConfig(const PlaylistUi::ColumnsConfig &arg);

    bool inhibitSleepWhilePlaying() const;
    void saveInhibitSleepWhilePlaying(bool arg);

    bool stopWhenTrackRemoved() const;
    void saveStopWhenTrackRemoved(bool arg);

    int playlistRowHeight() const;
    void savePlaylistRowHeight(int arg);

    QStringList mprisBlacklist() const;
    bool saveMprisBlacklist(const QStringList &arg);

    bool mpdStopPlayerOnClose() const;
    void saveMpdStopPlayerOnClose(bool arg);

    QStringList lyricsProviders() const;
    bool saveLyricsProviders(const QStringList &arg);

    QString libraryFilterScope() const;
    void saveLibraryFilterScope(const QString &arg);

  private:
    Config::Storage storage;
  };
}

#endif // GLOBAL_H
