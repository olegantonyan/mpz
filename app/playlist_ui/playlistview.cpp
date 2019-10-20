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

    connect(view, &QTableView::activated, [=](const QModelIndex &index) {
      auto i = TrackWrapper(model->itemAt(index), index.row(), model->current_playlist_index());
      emit activated(i);
    });
  }

  void View::on_load(const std::shared_ptr<Playlist> pi, int playlist_index) {
    model->setTracks(pi->tracks(), playlist_index);
  }

  void View::on_unload() {
    model->setTracks(QVector<Track>(), 0);
  }

  void View::on_prev_requested(const TrackWrapper &track) {
    int current = track.track_index;
    if (model->tracksSize() <= 0) {
      return;
    }
    auto prev = current - 1;
    if (prev < 0) {
      auto max = model->tracksSize() - 1;
      auto i = TrackWrapper(model->itemAt(model->buildIndex(max)), max, model->current_playlist_index());
      emit activated(i);
    } else {
      auto i = TrackWrapper(model->itemAt(model->buildIndex(prev)), prev, model->current_playlist_index());
      emit activated(i);
    }
  }

  void View::on_next_requested(const TrackWrapper &track) {
    int current = track.track_index;
    if (model->tracksSize() <= 0) {
      return;
    }
    auto next = current + 1;
    if (next > model->tracksSize() - 1) {
      auto i = TrackWrapper(model->itemAt(model->buildIndex(0)), 0, model->current_playlist_index());
      emit activated(i);
    } else {
      auto i = TrackWrapper(model->itemAt(model->buildIndex(next)), next, model->current_playlist_index());
      emit activated(i);
    }
  }

  void View::on_started(const TrackWrapper &track) {
    qDebug() << "started track" << track.track.filename();
    if (track.plalist_index == model->current_playlist_index()) {
      qDebug() << "correct playlist";
    } else {
      qDebug() << "wrong playlist";
    }
  }

  void View::on_stopped() {
    qDebug() << "stopped";
  }

  void View::on_resize() {
    int total_width = view->width();
    view->setColumnWidth(0, static_cast<int>(total_width * 0.28));
    view->setColumnWidth(1, static_cast<int>(total_width * 0.28));
    view->setColumnWidth(2, static_cast<int>(total_width * 0.28));
    view->setColumnWidth(3, static_cast<int>(total_width * 0.05));
    //view->setColumnWidth(4, total_width * 0.05);
    view->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
  }
}
