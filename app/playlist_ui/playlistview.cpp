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
  }

  void View::on_load(const Playlist &pi) {
    //qDebug() << "loading playlist" << pi.name();
    /*foreach (auto i, pi.tracks()) {
      qDebug() << i.path() << i.artist() << i.album() << i.title() << i.year();
    }*/
    model->setTracks(pi.tracks());
  }
}
