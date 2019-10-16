#include "playlistview.h"

#include <QDebug>
#include <QHeaderView>

namespace PlaylistUi {
  View::View(QTableView *v, QObject *parent) : QObject(parent) {
    view = v;
    model = new Model(this);
    view->setModel(model);
    view->horizontalHeader()->hide();
    view->verticalHeader()->hide();
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setShowGrid(false);
    //view->setFocusPolicy(Qt::NoFocus);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);

    //view->setColumnWidth(0, 400);

   // qDebug() << view->columnWidth(0);
  }

  void View::on_load(const std::shared_ptr<Playlist> pi) {
    model->setTracks(pi->tracks());
  }

  void View::on_unload() {
    model->setTracks(QVector<Track>());
  }
}
