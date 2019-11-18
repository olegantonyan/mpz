#include "playlistview.h"

#include <QDebug>
#include <QHeaderView>
#include <QAbstractItemView>

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

    view->setAlternatingRowColors(true);

    for (int c = 0; c < view->horizontalHeader()->count(); ++c) {
      view->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Fixed);
    }

    auto interceptor = new EventInterceptor(&PlaylistUi::View::on_event, this);
    view->installEventFilter(interceptor);

    connect(view, &QTableView::activated, [=](const QModelIndex &index) {
      emit activated(model->itemAt(index));
    });

    connect(view->selectionModel(), &QItemSelectionModel::currentChanged, [=](const QModelIndex &index, const QModelIndex &prev) {
      (void)prev;
      if (index.isValid()) {
        emit selected(model->itemAt(index));
      }
    });
  }

  void View::on_load(const std::shared_ptr<Playlist> pi) {
    model->setTracks(pi->tracks());
  }

  void View::on_unload() {
    model->setTracks(QVector<Track>());
  }

  void View::highlight(quint64 track_uid) {
    model->highlight(track_uid);
  }

  void View::on_stop() {
    highlight(0);
  }

  void View::on_start(const Track &t) {
    model->highlight(t.uid());
  }

  void View::on_scrollTo(const Track &track) {
    QModelIndex index = model->indexOf(track.uid());
    if (index.isValid()) {
      view->setCurrentIndex(index);
      view->scrollTo(index, QAbstractItemView::PositionAtCenter);
      emit selected(track);
    }
  }

  void View::on_event(QEvent *event) {
    if (event->type() == QEvent::Resize) {
      int total_width = view->width();
      view->horizontalHeader()->setMinimumSectionSize(20);
      view->setColumnWidth(0, 20);
      view->setColumnWidth(1, static_cast<int>(total_width * 0.28));
      view->setColumnWidth(2, static_cast<int>(total_width * 0.28));
      view->setColumnWidth(3, static_cast<int>(total_width * 0.28));
      view->setColumnWidth(4, static_cast<int>(total_width * 0.05));
      //view->setColumnWidth(4, total_width * 0.05);
      view->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    }
  }
}
