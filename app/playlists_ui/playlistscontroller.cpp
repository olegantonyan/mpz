#include "playlistscontroller.h"
#include "playlist.h"

#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QtConcurrent>
#include <QMouseEvent>

namespace PlaylistsUi {
  Controller::Controller(QListView *v, QLineEdit *s, Config::Local &conf, BusySpinner *_spinner, QObject *parent) :
    QObject(parent),
    view(v),
    search(s),
    local_conf(conf),
    spinner(_spinner) {

    model = new PlaylistsUi::Model(conf, this);
    spinner->show();
    connect(model, &Model::asynLoadFinished, spinner, &BusySpinner::hide);
    connect(model, &Model::asynLoadFinished, this, &Controller::load);
    model->loadAsync();

    proxy = new ProxyFilterModel(model, this);

    view->setContextMenuPolicy(Qt::CustomContextMenu);
    view->setSelectionMode(QAbstractItemView::NoSelection);

    connect(view, &QListView::clicked, this, &Controller::on_itemActivated);

    view->viewport()->installEventFilter(this);

    connect(search, &QLineEdit::textChanged, this, &Controller::on_search);
    search->setClearButtonEnabled(true);

    context_menu = new PlaylistsContextMenu(model, proxy, view, search, this);
    connect(context_menu, &PlaylistsContextMenu::removed, this, &Controller::on_removeItem);

    view->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(view, &QListView::customContextMenuRequested, context_menu, &PlaylistsContextMenu::show);
  }

  void Controller::load() {
    view->setModel(proxy);
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
          on_removeItem(index);
        }
      }
      if (me->button() == Qt::BackButton) {
        search->clear();
      }
    }
    return QObject::eventFilter(obj, event);
  }

  void Controller::persist(int current_index) {
    auto max_index = qMax(model->listSize() - 1, 0);
    auto save_index = qMin(current_index, max_index);
    local_conf.saveCurrentPlaylist(save_index);
  }

  void Controller::on_removeItem(const QModelIndex &index) {
    model->remove(proxy->mapToSource(index));
    if (view->selectionModel()->selectedIndexes().size() > 0) {
      auto selected_idx = view->selectionModel()->selectedIndexes().first();
      if (selected_idx == index || model->listSize() == 1) {
        on_itemActivated(model->buildIndex(0));
      }
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

    on_itemActivated(proxy->mapFromSource(model->itemIndex(playlist)));
  }

  void Controller::on_playlistChanged(const std::shared_ptr<Playlist> pl) {
    Q_UNUSED(pl)
    model->persist();
  }

  void Controller::on_itemActivated(const QModelIndex &index) {
    if (model->listSize() <= 0) {
      return;
    }
    auto source_index = proxy->mapToSource(index);
    auto item = model->itemAt(source_index);
    persist(source_index.row());
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
    proxy->setFilterWildcard("*" + term + "*");

    QTimer::singleShot(20, [=]() {
      if (!view->selectionModel()->selectedRows().isEmpty()) {
        view->scrollTo(view->selectionModel()->selectedRows().first(), QAbstractItemView::PositionAtCenter);
      }
    });
  }
}
