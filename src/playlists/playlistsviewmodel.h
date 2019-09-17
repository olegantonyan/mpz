#ifndef PLAYLISTSVIEWMODEL_H
#define PLAYLISTSVIEWMODEL_H

#include <QObject>
#include <QListView>
#include <QString>
#include <QStringListModel>

namespace Playlists {
  class PlaylistsViewModel : public QObject {
    Q_OBJECT

  public:
    PlaylistsViewModel(QListView *view, QObject *parent = nullptr);

  public slots:
    void on_createPlaylist(const QString &filepath);

  private:
    QListView *view;
    QStringListModel *model;
  };
}

#endif // PLAYLISTSVIEWMODEL_H
