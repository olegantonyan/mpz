#ifndef PLAYLISTSVIEWMODEL_H
#define PLAYLISTSVIEWMODEL_H

#include "playlistsmodel.h"
#include "playlist/playlist.h"
#include "config/local.h"
#include "track.h"
#include "busyspinner.h"
#include "playlistsproxyfiltermodel.h"
#include "playlistscontextmenu.h"

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
    std::shared_ptr<Playlist::Playlist> playlistByTrackUid(quint64 track_uid) const;

  public slots:
    void on_createPlaylist(const QList<QDir> &filepaths);
    void on_jumpTo(const std::shared_ptr<Playlist::Playlist> playlist);
    void on_playlistChanged(const std::shared_ptr<Playlist::Playlist> pl);
    void on_start(const Track &t);
    void on_stop();

  signals:
    void selected(const std::shared_ptr<Playlist::Playlist> item);
    void loaded(const std::shared_ptr<Playlist::Playlist> item);
    void emptied();
    void doubleclicked(const std::shared_ptr<Playlist::Playlist> item);

  private slots:
    void on_itemActivated(const QModelIndex &index);
    void on_playlistLoadFinished(Playlist::Playlist *pl);
    void on_search(const QString& term);
    void load();
    void on_removeItem(const QModelIndex &index);
    void on_itemDoubleClicked(const QModelIndex &index);

  private:
    QListView *view;
    QLineEdit *search;
    Model *model;
    Config::Local &local_conf;
    BusySpinner *spinner;
    ProxyFilterModel *proxy;
    PlaylistsContextMenu *context_menu;

    void persist(int current_index);

    void eventFilterTableView(QEvent *event);
    void eventFilterViewport(QEvent *event);

  protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
  };
}

#endif // PLAYLISTSVIEWMODEL_H
