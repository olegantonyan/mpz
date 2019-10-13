#include "playlistview.h"

#include <QDebug>
#include <QHeaderView>

namespace PlaylistUi {
  View::View(QTableView *v, Config::Local &conf, QObject *parent) : QObject(parent), local_conf(conf) {
    view = v;
    model = new Model(this);
    view->setModel(model);
    view->horizontalHeader()->hide();
    view->verticalHeader()->hide();
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setShowGrid(false);
    //view->setFocusPolicy(Qt::NoFocus);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
  }

  void View::on_load(const std::shared_ptr<Playlist> pi) {
    model->setTracks(pi->tracks());
  }

  void View::on_unload() {
    model->setTracks(QVector<Track>());
  }
}
