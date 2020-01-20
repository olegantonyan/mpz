#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include "playlistmodel.h"
#include "playlist.h"
#include "track.h"
#include "config/local.h"
#include "playlistproxyfiltermodel.h"

#include <QObject>
#include <QTableView>
#include <memory>
#include <QEvent>
#include <QLineEdit>
#include <QHash>

namespace PlaylistUi {
  class Controller : public QObject {
    Q_OBJECT
  public:
    explicit Controller(QTableView *v, QLineEdit *search, Config::Local &local_cfg, QObject *parent = nullptr);

  signals:
    void activated(const Track &track);
    void selected(const Track &track);
    void changed(const std::shared_ptr<Playlist> pl);

  public slots:
    void on_load(const std::shared_ptr<Playlist> pi);
    void on_unload();
    void on_stop();
    void on_start(const Track &t);
    void on_pause(const Track &t);
    void on_scrollTo(const Track &track);
    void on_appendToPlaylist(const QDir &filepath);

  private slots:
    void on_appendAsyncFinished(Playlist *pl);
    void on_contextMenu(const QPoint &pos);
    void on_search(const QString &term);
    
  private:
    QTableView *view;
    QLineEdit *search;
    Model *model;
    Config::Local &local_conf;
    bool restore_scroll_once;
    QHash<quint64,int> scroll_positions;
    ProxyFilterModel *proxy;
    
  protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
  };
}

#endif // PLAYLISTVIEW_H
