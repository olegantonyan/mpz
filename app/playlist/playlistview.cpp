#include "playlistview.h"

#include <QHeaderView>

namespace Playlist {
  View::View(QTableView *v, QObject *parent) : QObject(parent) {
    view = v;
    model = new Playlist::Model(this);
    view->setModel(model);
    view->horizontalHeader()->hide();
    view->verticalHeader()->hide();
  }
}
