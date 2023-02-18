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

    int ipcPort() const;
    bool saveIpcPort(int arg);

    bool singleInstance() const;

    int playbackLogSize() const;

    PlaylistUi::ColumnsConfig columnsConfig() const;
    bool saveColumnsConfig(const PlaylistUi::ColumnsConfig &arg);

    bool inhibitSleepWhilePlaying() const;

    int playlistRowHeight() const;

  private:
    Config::Storage storage;
  };
}

#endif // GLOBAL_H
