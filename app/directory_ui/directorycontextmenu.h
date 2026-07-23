#ifndef DIRECTORYCONTEXTMENU_H
#define DIRECTORYCONTEXTMENU_H

#include "directorymodel/proxy.h"
#include "track.h"
#include "playlist/playlist.h"

#include <QObject>
#include <QPoint>
#include <QTreeView>
#include <QLineEdit>
#include <memory>
#include <QAction>
#include <QDir>

namespace DirectoryUi {
  class DirectoryContextMenu : public QObject {
    Q_OBJECT
  public:
    explicit DirectoryContextMenu(DirectoryModel::Proxy *model, QTreeView *view, QLineEdit *seacrh, QObject *parent = nullptr);

  public slots:
    void show(const QPoint &pos);

  signals:
    void createNewPlaylist(const QList<QDir> &filepaths);
    void appendToCurrentPlaylist(const QList<QDir> &filepaths);
    void createNewPlaylistFromTracks(const QVector<Track> &tracks, const QString &name);
    void appendTracksToCurrentPlaylist(const QVector<Track> &tracks);
    void editStations();

  private:
    void showRadioMenu(const QPoint &pos, const QModelIndex &index);

    DirectoryModel::Proxy *model;
    QTreeView *view;
    QLineEdit *search;
  };
}

#endif // DIRECTORYCONTEXTMENU_H
