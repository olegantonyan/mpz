#ifndef PLAYLISTSVIEWMODEL_H
#define PLAYLISTSVIEWMODEL_H

#include "playlistsmodel.h"
#include "playlist.h"
#include "config/local.h"
#include "track.h"
#include "busyspinner.h"
#include "playlistsproxyfiltermodel.h"

#include <QObject>
#include <QListView>
#include <QString>
#include <QPoint>
#include <QDir>
#include <QModelIndex>
#include <memory>
#include <QLineEdit>

namespace PlaylistsUi {
  class Controller : public QObject {
    Q_OBJECT

  public:
    explicit Controller(QListView *view, QLineEdit *search, Config::Local &conf, BusySpinner *_spinner, QObject *parent = nullptr);
    std::shared_ptr<Playlist> playlistByTrackUid(quint64 track_uid) const;

  public slots:
    void on_createPlaylist(const QDir &filepath);
    void on_jumpTo(const std::shared_ptr<Playlist> playlist);
    void on_playlistChanged(const std::shared_ptr<Playlist> pl);

  signals:
    void selected(const std::shared_ptr<Playlist> item);
    void emptied();

  private slots:
    void on_customContextMenuRequested(const QPoint &pos);
    void on_itemActivated(const QModelIndex &index);
    void on_playlistLoadFinished(Playlist *pl);
    void on_search(const QString& term);
    void load();

  private:
    QListView *view;
    QLineEdit *search;
    Model *model;
    Config::Local &local_conf;
    BusySpinner *spinner;
    ProxyFilterModel *proxy;

    void persist(int current_index);

    void removeItem(const QModelIndex &idx);

  protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
  };
}

#endif // PLAYLISTSVIEWMODEL_H
