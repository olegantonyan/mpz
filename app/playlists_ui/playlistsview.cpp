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
      auto idx = model->buildIndex(qMin(local_conf.currentPlaylist(), model->listSize() - 1));
      auto item = model->itemAt(idx);
      view->setCurrentIndex(idx);
      view->selectionModel()->select(idx, {QItemSelectionModel::Select});
      emit selected(item);
    }
  }

  void View::persist(int current_index) {
    auto max_index = qMax(model->listSize() - 1, 0);
    auto save_index = qMin(current_index, max_index);
    local_conf.saveCurrentPlaylist(save_index);
  }

  void View::on_createPlaylist(const QDir &filepath) {
    auto pl = new Playlist();
    pl->load(filepath);
    auto item = std::shared_ptr<Playlist>(pl);
    auto index = model->append(item);
    view->setCurrentIndex(index);
    view->selectionModel()->clearSelection();
    view->selectionModel()->select(index, {QItemSelectionModel::Select});
    persist(index.row());
    emit selected(item);
  }

  void View::on_appendToCurrentPlaylist(const QDir &filepath) {
    qDebug() << filepath;
  }

  void View::on_trackSelected(const Track &track) {
    state.setSelected(track.uid());
  }

  void View::on_prevRequested() {
    quint64 current_track_uid = state.playingTrack();
    auto current_playlist = model->itemByTrack(current_track_uid);
    if (current_playlist == nullptr) {
      return;
    }

    int current = current_playlist->trackIndex(current_track_uid);
    auto prev = current - 1;
    if (prev < 0) {
      auto max = current_playlist->tracks().size() - 1;
      Track t = current_playlist->tracks().at(max);
      emit activated(t);
    } else {
      Track t = current_playlist->tracks().at(prev);
      emit activated(t);
    }
  }

  void View::on_nextRequested() {
    quint64 current_track_uid = state.playingTrack();
    auto current_playlist = model->itemByTrack(current_track_uid);
    if (current_playlist == nullptr) {
      return;
    }

    int current = current_playlist->trackIndex(current_track_uid);
    auto next = current + 1;
    if (next > current_playlist->tracks().size() - 1) {
      Track t = current_playlist->tracks().at(0);
      emit activated(t);
    } else {
      Track t = current_playlist->tracks().at(next);
      emit activated(t);
    }
  }

  void View::on_startRequested() {
    quint64 selected_track_uid = state.selectedTrack();
    if (selected_track_uid == 0) {
      return;
    }
    auto selected_playlist = model->itemByTrack(selected_track_uid);
    if (selected_playlist == nullptr) {
      return;
    }
    Track t = selected_playlist->trackBy(selected_track_uid);
    emit activated(t);
  }

  void View::on_started(const Track &track) {
    on_stopped();
    state.setPlaying(track.uid());
  }

  void View::on_stopped() {
    state.resetPlaying();
  }

  void View::on_jumpToCurrent() {
    quint64 current_track_uid = state.playingTrack();
    auto current_playlist = model->itemByTrack(current_track_uid);
    if (current_playlist == nullptr) {
      return;
    }
    on_itemActivated(model->itemIndex(current_playlist));
    emit scrolling(current_playlist->trackBy(current_track_uid));
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
    persist(index.row());
    view->selectionModel()->clearSelection();
    view->selectionModel()->select(index, {QItemSelectionModel::Select});
    emit selected(item);
  }
}
