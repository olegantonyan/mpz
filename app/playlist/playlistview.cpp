#include "playlistview.h"

#include <QDebug>
#include <QHeaderView>

namespace Playlist {
  View::View(QTableView *v, QObject *parent) : QObject(parent) {
    view = v;
    model = new Playlist::Model(this);
    view->setModel(model);
    view->horizontalHeader()->hide();
    view->verticalHeader()->hide();
  }

  void View::on_load(const Playlists::PlaylistItem &pi) {
    qDebug() << "loading playlist" << pi.getName();
  }
}
