#include "playlistscontroller.h"
#include "playlist.h"

#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QtConcurrent>

namespace PlaylistsUi {
  Controller::Controller(QListView *v, QLineEdit *s, Config::Local &conf, BusySpinner *_spinner, QObject *parent) :
    QObject(parent),
    view(v),
    search(s),
    local_conf(conf),
    spinner(_spinner) {
    model = new PlaylistsUi::Model(conf, this);
    connect(model, &Model::asynLoadStarted, spinner, &BusySpinner::show);
    connect(model, &Model::asynLoadFinished, spinner, &BusySpinner::hide);
    connect(model, &Model::asynLoadFinished, this, &Controller::load);
    model->loadAsync();

    view->setModel(model);
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    view->setSelectionMode(QAbstractItemView::NoSelection);

    connect(view, &QListView::customContextMenuRequested, this, &Controller::on_customContextMenuRequested);
    connect(view, &QListView::clicked, this, &Controller::on_itemActivated);

    view->viewport()->installEventFilter(this);

    connect(search, &QLineEdit::textChanged, this, &Controller::on_search);
    search->setClearButtonEnabled(true);
  }

  void Controller::load() {
    if (model->listSize() > 0) {
      auto idx = model->buildIndex(qMin(local_conf.currentPlaylist(), model->listSize() - 1));
      auto item = model->itemAt(idx);
      view->setCurrentIndex(idx);
      view->selectionModel()->select(idx, {QItemSelectionModel::Select});
      emit selected(item);
    }
  }

  std::shared_ptr<Playlist> Controller::playlistByTrackUid(quint64 track_uid) const {
    return model->itemByTrack(track_uid);
  }

  bool Controller::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseButtonRelease) {
      QMouseEvent *me = dynamic_cast<QMouseEvent *>(event);
      if (me->button() == Qt::MidButton) {
        auto index = view->indexAt(me->pos());
        if (index.isValid()) {
          removeItem(index);
        }
      }
    }
    return QObject::eventFilter(obj, event);
  }

  void Controller::persist(int current_index) {
    auto max_index = qMax(model->listSize() - 1, 0);
    auto save_index = qMin(current_index, max_index);
    local_conf.saveCurrentPlaylist(save_index);
  }

  void Controller::removeItem(const QModelIndex &index) {
    model->remove(index);
    auto selected_idx = view->selectionModel()->selectedIndexes().first();
    if (selected_idx == index || model->listSize() == 1) {
      on_itemActivated(model->buildIndex(0));
    }
    if (model->listSize() == 0) {
      emit emptied();
    }
  }

  void Controller::on_createPlaylist(const QDir &filepath) {
      auto pl = new Playlist();
      connect(pl, &Playlist::loadAsyncFinished, this, &Controller::on_playlistLoadFinished);
      pl->loadAsync(filepath);
      spinner->show();
  }

  void Controller::on_jumpTo(const std::shared_ptr<Playlist> playlist) {
    if (playlist == nullptr) {
      return;
    }
    on_itemActivated(model->itemIndex(playlist));
  }

  void Controller::on_playlistChanged(const std::shared_ptr<Playlist> pl) {
    Q_UNUSED(pl)
    model->persist();
  }

  void Controller::on_customContextMenuRequested(const QPoint &pos) {
    auto index = view->indexAt(pos);
    if (!index.isValid()) {
      return;
    }

    QMenu menu;
    QAction remove("Remove");
    QAction rename("Rename");

    connect(&remove, &QAction::triggered, [=]() {
      removeItem(index);
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

  void Controller::on_itemActivated(const QModelIndex &index) {
    if (model->listSize() <= 0) {
      return;
    }
    auto item = model->itemAt(index);
    persist(index.row());
    view->selectionModel()->clearSelection();
    view->selectionModel()->select(index, {QItemSelectionModel::Select});
    emit selected(item);
  }

  void Controller::on_playlistLoadFinished(Playlist *pl) {
    disconnect(pl, &Playlist::loadAsyncFinished, this, &Controller::on_playlistLoadFinished);
    auto item = std::shared_ptr<Playlist>(pl);
    auto index = model->append(item);
    view->setCurrentIndex(index);
    view->selectionModel()->clearSelection();
    view->selectionModel()->select(index, {QItemSelectionModel::Select});
    persist(index.row());
    emit selected(item);
    spinner->hide();
  }

  void Controller::on_search(const QString &term) {
    if (model->listSize() == 0 || model->itemList().size() == 0) {
      return;
    }
    view->selectionModel()->clear();
    if (term.isEmpty()) {
      return;
    }

    for (int i = 0; i < model->itemList().size(); i++) {
      auto t = model->itemList().at(i);
      if (t->name().contains(term, Qt::CaseInsensitive)) {
        view->selectionModel()->select(model->index(i), QItemSelectionModel::Select); // TODO: rewrite to select all required rows at once
        QThread::currentThread()->yieldCurrentThread();
      }
    }
  }
}
