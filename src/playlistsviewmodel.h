#ifndef PLAYLISTSVIEWMODEL_H
#define PLAYLISTSVIEWMODEL_H

#include <QObject>
#include <QListView>
#include <QString>

class PlaylistsViewModel : public QObject {
  Q_OBJECT

public:
  PlaylistsViewModel(QListView *view, QObject *parent = nullptr);

public slots:
  void on_createPlaylist(const QString &filepath);

private:
  QListView *view;
};

#endif // PLAYLISTSVIEWMODEL_H
