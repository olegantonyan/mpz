#include "playlistsview.h"
#include "playlist.h"

#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QItemSelectionModel>

namespace PlaylistsUi {
  View::View(QListView *v, Config::Local &conf, QObject *parent) : QObject(parent), local_conf(conf){
    view = v;
    model = new PlaylistsUi::Model(conf, this);

    view->setModel(model);
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    view->setSelectionMode(QAbstractItemView::NoSelection);

    connect(view, &QListView::customContextMenuRequested, this, &View::on_customContextMenuRequested);
    connect(view, &QListView::clicked, this, &View::on_itemActivated);
  }

  void View::load() {
    if (model->listSize() > 0) {
      auto idx = model->buildIndex(qMax(local_conf.currentPlaylist(), model->listSize() - 1));
      auto item = model->itemAt(idx);
      view->setCurrentIndex(idx);
      view->selectionModel()->select(idx, {QItemSelectionModel::Select});
      state.setSelectedPlaylist(idx.row());
      emit selected(item);
    }
  }

  void View::persist(int current_index) {
    local_conf.saveCurrentPlaylist(qMax(current_index, qMax(model->listSize() - 1, 0)));
  }

  void View::on_createPlaylist(const QDir &filepath) {
    auto pl = new Playlist();
    pl->load(filepath);
    auto item = std::shared_ptr<Playlist>(pl);
    auto index = model->append(item);
    view->setCurrentIndex(index);
    view->selectionModel()->clearSelection();
    view->selectionModel()->select(index, {QItemSelectionModel::Select});
    state.setSelectedPlaylist(index.row());

    persist(index.row());
    emit selected(item);
  }

  void View::on_trackActivated(Track track, int index) {
    emit activated(TrackWrapper(track, index, state.selected().playlist_index));
  }

  void View::on_trackSelected(Track track, int index) {
    Q_UNUSED(track)
    state.setSelectedTrack(index);
  }

  void View::on_prevRequested() {
    int current = state.playing().track_index;
    if (current < 0 || state.playing().playlist_index < 0) {
      return;
    }

    auto pl = model->itemAt(model->buildIndex(state.playing().playlist_index));
    if (pl->tracks().size() <= 0) {
      return;
    }
    auto prev = current - 1;
    if (prev < 0) {
      auto max = pl->tracks().size() - 1;
      auto i = TrackWrapper(pl->tracks().at(max), max, state.playing().playlist_index);
      emit activated(i);
    } else {
      auto i = TrackWrapper(pl->tracks().at(prev), prev, state.playing().playlist_index);
      emit activated(i);
    }
  }

  void View::on_nextRequested() {
    int current = state.playing().track_index;
    if (current < 0 || state.playing().playlist_index < 0) {
      return;
    }

    //qDebug() << "next" << state.playing().track_index << state.playing().playlist_index;
    auto pl = model->itemAt(model->buildIndex(state.playing().playlist_index));
    if (pl->tracks().size() <= 0) {
      return;
    }
    auto next = current + 1;
    if (next > pl->tracks().size() - 1) {
      auto i = TrackWrapper(pl->tracks().first(), 0, state.playing().playlist_index);
      emit activated(i);
    } else {
      auto i = TrackWrapper(pl->tracks().at(next), next, state.playing().playlist_index);
      emit activated(i);
    }
  }

  void View::on_startRequested() {
    int t_index = state.selected().track_index;
    int p_index = state.selected().playlist_index;
    if (t_index < 0 || p_index < 0) {
      return;
    }

    auto pl = model->itemAt(model->buildIndex(p_index));
    if (pl->tracks().size() >= t_index) {
      emit activated(TrackWrapper(pl->tracks().at(t_index), t_index, p_index));
    }
  }

  void View::on_started(TrackWrapper track) {
    on_stopped();
    state.setPlayingTrack(track.track_index);
    state.setPlayingPlaylist(track.playlist_index);
    if (track.playlist_index == state.selected().playlist_index) {
      emit highlighted(track.track_index);
    }
  }

  void View::on_stopped() {
    state.resetPlaying();
    emit highlighted(-1);
  }

  void View::on_customContextMenuRequested(const QPoint &pos) {
    auto index = view->indexAt(pos);
    if (!index.isValid()) {
      return;
    }

    QMenu menu;
    QAction remove("Remove");
    QAction rename("Rename");

    connect(&remove, &QAction::triggered, [=]() {
      model->remove(index);
      auto selected_idx = view->selectionModel()->selectedIndexes().first();
      if (selected_idx == index || model->listSize() == 1) {
        on_itemActivated(model->buildIndex(0));
      }
      if (model->listSize() == 0) {
        emit emptied();
      }
    });
    connect(&rename, &QAction::triggered, [=]() {
      auto i = model->itemAt(index);
      bool ok;
      QString new_name = QInputDialog::getText(view, QString("Rename playlist '%1'").arg(i->name()), "", QLineEdit::Normal, i->name(), &ok, Qt::Widget);
      if (ok && !new_name.isEmpty()) {
        i->rename(new_name);
      }
    });

    menu.addAction(&rename);
    menu.addAction(&remove);
    menu.exec(view->viewport()->mapToGlobal(pos));
  }

  void View::on_itemActivated(const QModelIndex &index) {
    if (model->listSize() <= 0) {
      return;
    }
    auto item = model->itemAt(index);
    if (model->itemAt(model->buildIndex(state.selected().playlist_index)) != item) {
      persist(index.row());
      view->selectionModel()->clearSelection();
      view->selectionModel()->select(index, {QItemSelectionModel::Select});
      emit selected(item);
    }
    state.setSelectedPlaylist(index.row());

    if (state.playing().playlist_index == index.row()) {
      emit highlighted(state.playing().track_index);
    }
  }
}
