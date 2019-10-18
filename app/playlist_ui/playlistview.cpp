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
    //view->horizontalHeader()->setStretchLastSection(true);

    for (int c = 0; c < view->horizontalHeader()->count(); ++c) {
      view->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Fixed);
    }

    auto interceptor = new ResizeEventInterceptor(&PlaylistUi::View::on_resize, this);
    view->installEventFilter(interceptor);
  }

  void View::on_load(const std::shared_ptr<Playlist> pi, int playlist_index) {
    model->setTracks(pi->tracks(), playlist_index);
  }

  void View::on_unload() {
    model->setTracks(QVector<Track>(), 0);
  }

  void View::on_resize() {
    int total_width = view->width() - 50;
    view->setColumnWidth(0, total_width * 0.3);
    view->setColumnWidth(1, total_width * 0.3);
    view->setColumnWidth(2, total_width * 0.3);
    view->setColumnWidth(3, total_width * 0.05);
    view->setColumnWidth(4, total_width * 0.05);
  }
}
