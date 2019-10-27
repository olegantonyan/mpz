#ifndef PLAYLISTSVIEWMODEL_H
#define PLAYLISTSVIEWMODEL_H

#include "playlistsmodel.h"
#include "playlist.h"
#include "config/local.h"
#include "playerstate.h"
#include "trackwrapper.h"

#include <QObject>
#include <QListView>
#include <QString>
#include <QPoint>
#include <QDir>
#include <QModelIndex>
#include <memory>

namespace PlaylistsUi {
  class View : public QObject {
    Q_OBJECT

  public:
    explicit View(QListView *view, Config::Local &conf, QObject *parent = nullptr);
    void load();

  public slots:
    void on_createPlaylist(const QDir &filepath);

    void on_trackActivated(Track track, int index);
    void on_trackSelected(Track track, int index);

    void on_prevRequested(TrackWrapper track);
    void on_nextRequested(TrackWrapper track);
    void on_startRequested();
    void on_started(TrackWrapper track);
    void on_stopped();

  signals:
    void selected(const std::shared_ptr<Playlist> item);
    void emptied();
    void activated(const TrackWrapper &track);
    void highlighted(int row);

  private slots:
    void on_customContextMenuRequested(const QPoint &pos);
    void on_itemActivated(const QModelIndex &index);

  private:
    QListView *view;
    Model *model;
    PlayerState state;

    Config::Local &local_conf;
    void persist(int current_index);
  };
}

#endif // PLAYLISTSVIEWMODEL_H
