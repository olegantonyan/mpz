#include "playlistscontroller.h"
#include "playlist/playlist.h"
#include "playlistsmodel.h"
#include "playlist/loader.h"
#include "dropdirs.h"

#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QtConcurrent>
#include <QMouseEvent>
#include <QDropEvent>
#include <QMimeData>

namespace PlaylistsUi {
  Controller::Controller(QListView *v, QLineEdit *s, Config::Local &conf, BusySpinner *_spinner, ModusOperandi &modus, QObject *parent) :
    QObject(parent),
    view(v),
    search(s),
    spinner(_spinner),
    modus_operandi(modus) {

    proxy = new ProxyFilterModel(conf, modus, this);
    spinner->show();
    connect(proxy, &ProxyFilterModel::asyncLoadFinished, spinner, &BusySpinner::hide);
    connect(proxy, &ProxyFilterModel::asyncLoadFinished, this, &Controller::load);
    connect(proxy, &ProxyFilterModel::createPlaylistAsyncFinished, this, &Controller::on_playlistLoadFinished);
    connect(proxy, &ProxyFilterModel::asyncTracksLoadFinished, this, &Controller::selected);

    view->setContextMenuPolicy(Qt::CustomContextMenu);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setDragEnabled(true);
    view->setAcceptDrops(true);
    view->setDropIndicatorShown(true);
    view->setDragDropMode(QAbstractItemView::InternalMove);
    view->setDefaultDropAction(Qt::MoveAction);

    connect(view, &QListView::doubleClicked, this, &Controller::on_itemDoubleClicked);

    view->viewport()->installEventFilter(this);
    view->installEventFilter(this);

    connect(search, &QLineEdit::textChanged, this, &Controller::on_search);
    search->setClearButtonEnabled(true);

    context_menu = new PlaylistsContextMenu(proxy, view, search, this);
    connect(context_menu, &PlaylistsContextMenu::removed, this, &Controller::on_removeItem);

    view->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(view, &QListView::customContextMenuRequested, context_menu, &PlaylistsContextMenu::show);

    connect(context_menu, &PlaylistsContextMenu::playlistChanged, this, &Controller::selected);
    connect(context_menu, &PlaylistsContextMenu::loadPlaylistFiles, this, &Controller::on_importPlayistFiles);
    connect(context_menu, &PlaylistsContextMenu::renamed, proxy, &ProxyFilterModel::onRename);

    connect(proxy, &PlaylistsUi::ProxyFilterModel::asyncLoadFinished, this, &PlaylistsUi::Controller::asyncLoadFinished);
  }

  void Controller::load() {
    view->setModel(proxy);
    if (proxy->activeModel()->listSize() > 0) {
      auto idx = proxy->activeModel()->currentPlaylistIndex();
      if (idx.isValid()) {
        auto item = proxy->activeModel()->itemAt(idx);
        if (item) {
          view->setCurrentIndex(proxy->mapFromSource(idx));
          view->selectionModel()->select(idx, {QItemSelectionModel::Select});
          proxy->activeModel()->asyncTracksLoad(item);
        }
      }
    }
  }

  std::shared_ptr<Playlist::Playlist> Controller::playlistByTrackUid(quint64 track_uid) const {
    return proxy->activeModel()->itemByTrack(track_uid);
  }

  std::shared_ptr<Playlist::Playlist> Controller::playlistByName(const QString &name) const {
    for (const auto &it : proxy->activeModel()->itemList()) {
      if (it->name() == name) {
        return it;
      }
    }
    return nullptr;
  }

  std::shared_ptr<Playlist::Playlist> Controller::currentPlaylist() const {
    return proxy->activeModel()->itemAt(proxy->activeModel()->currentPlaylistIndex());
  }

  int Controller::playlistsCount() const {
    return proxy->activeModel()->listSize();
  }

  bool Controller::eventFilter(QObject *obj, QEvent *event) {
    if (obj == view->viewport()) {
      if (handleExternalDnd(event)) {
        return true;
      }
      eventFilterViewport(event);
    } else if (obj == view) {
      eventFilterTableView(event);
    }
    return QObject::eventFilter(obj, event);
  }

  bool Controller::handleExternalDnd(QEvent *event) {
    const auto type = event->type();
    if (type != QEvent::DragEnter && type != QEvent::DragMove && type != QEvent::Drop) {
      return false;
    }
    if (modus_operandi.get() != ModusOperandi::MODUS_LOCALFS) {
      return false;
    }
    auto *drop_event = static_cast<QDropEvent *>(event);
    if (!drop_event->mimeData()->hasUrls()) {
      return false;
    }
    if (type == QEvent::Drop) {
      onExternalDrop(drop_event);
    }
    drop_event->acceptProposedAction();
    return true;
  }

  void Controller::onExternalDrop(QDropEvent *event) {
    const auto dirs = DropUtil::droppedDirs(event->mimeData());
    if (dirs.isEmpty()) {
      return;
    }

    const QString libraryDir = DropUtil::commonParentDir(dirs);
    const auto index = view->indexAt(DropUtil::dropPosition(event));
    if (!index.isValid()) {
      on_createPlaylist(dirs, libraryDir);
      return;
    }

    auto playlist = proxy->itemAt(index);
    if (!playlist) {
      return;
    }

    QMenu menu(view);
    auto *create_action = menu.addAction(tr("Create new playlist"));
    auto *append_action = menu.addAction(tr("Append to \"%1\"").arg(playlist->name()));
    auto *chosen = menu.exec(view->viewport()->mapToGlobal(DropUtil::dropPosition(event)));

    if (chosen == create_action) {
      on_createPlaylist(dirs, libraryDir);
    } else if (chosen == append_action) {
      QStringList paths;
      for (const auto &dir : std::as_const(dirs)) {
        paths << dir.absolutePath();
      }
      on_importPlayistFiles(index, paths);
    }
  }

  void Controller::eventFilterTableView(QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
      QKeyEvent* keyevent = dynamic_cast<QKeyEvent*>(event);
      if (keyevent->key() == Qt::Key_Delete
#ifdef Q_OS_MACOS
          || keyevent->key() == Qt::Key_Backspace
#endif
         ) {
        for (const auto &i : view->selectionModel()->selectedIndexes()) {
          on_removeItem(i);
        }
      } else if (keyevent->key() == Qt::Key_F2) {
        auto selected = view->selectionModel()->selectedIndexes();
        if (!selected.isEmpty() && selected.first().isValid()) {
          context_menu->on_rename(selected.first());
        }
      } else if (keyevent->key() == Qt::Key_Return) {
        on_itemActivated(view->currentIndex());
      }
    }
  }

  void Controller::eventFilterViewport(QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
      QMouseEvent *me = dynamic_cast<QMouseEvent *>(event);
      if (me->button() == Qt::LeftButton) {
        auto index = view->indexAt(me->pos());
        if (index.isValid()) {
          on_itemActivated(index);
        }
      }
    }

    if (event->type() == QEvent::MouseButtonRelease) {
      QMouseEvent *me = dynamic_cast<QMouseEvent *>(event);
      if (me->button() == Qt::MiddleButton) {
        auto index = view->indexAt(me->pos());
        if (index.isValid()) {
          on_removeItem(index);
        }
      }
      if (me->button() == Qt::BackButton) {
        search->clear();
      }
    }
  }

  void Controller::on_removeItem(const QModelIndex &index) {
    proxy->activeModel()->remove(proxy->mapToSource(index));
    emit removed();
    if (view->selectionModel()->selectedIndexes().size() > 0) {
      auto selected_idx = view->selectionModel()->selectedIndexes().first();
      if (selected_idx == index || proxy->activeModel()->listSize() == 1) {
        on_itemActivated(proxy->activeModel()->buildIndex(0));
      }
    }
    if (proxy->activeModel()->listSize() == 0) {
      emit emptied();
    }
  }

  void Controller::on_itemDoubleClicked(const QModelIndex &index) {
    if (proxy->activeModel()->listSize() <= 0) {
      return;
    }
    auto item = proxy->itemAt(index);
    emit doubleclicked(item);
  }

  void Controller::on_importPlayistFiles(const QModelIndex &index, const QStringList &filespaths) {
    if (proxy->activeModel()->listSize() <= 0 || !index.isValid()) {
      return;
    }
    auto playlist = proxy->itemAt(index);

    QVector<Track> tracks;
    QStringList keys;
    for (const auto &it : std::as_const(filespaths)) {
      Playlist::Loader ldr(it);
      for (const auto &track : ldr.tracks()) {
        const auto key = track.url().toString();
        if (!keys.contains(key)) {
          tracks.append(track);
          keys << key;
        }
      }
    }
    proxy->activeModel()->appendTracksToPlaylist(playlist, tracks);
    on_playlistChanged(playlist);
  }

  void Controller::on_start(const Track &t) {
    proxy->activeModel()->higlight(proxy->activeModel()->itemByTrack(t.uid()));
  }

  void Controller::on_stop() {
    proxy->activeModel()->higlight(nullptr);
  }

  void Controller::on_createPlaylist(const QList<QDir> &filepaths, const QString &libraryDir) {
    proxy->activeModel()->createPlaylistAsync(filepaths, libraryDir);
    spinner->show();
  }

  void Controller::on_jumpTo(const std::shared_ptr<Playlist::Playlist> playlist) {
    if (playlist == nullptr) {
      return;
    }

    on_itemActivated(proxy->mapFromSource(proxy->activeModel()->itemIndex(playlist)));
  }

  void Controller::on_playlistChanged(const std::shared_ptr<Playlist::Playlist> pl) {
    Q_UNUSED(pl)
    proxy->activeModel()->persist();
  }

  void Controller::on_itemActivated(const QModelIndex &index) {
    if (proxy->activeModel()->listSize() <= 0) {
      return;
    }
    auto source_index = proxy->mapToSource(index);
    auto item = proxy->activeModel()->itemAt(source_index);
    proxy->activeModel()->saveCurrentPlaylistIndex(source_index);
    view->selectionModel()->clearSelection();
    view->selectionModel()->select(index, {QItemSelectionModel::Select});
    proxy->activeModel()->asyncTracksLoad(item);
  }

  void Controller::on_playlistLoadFinished(std::shared_ptr<Playlist::Playlist> pl) {
    auto index = proxy->append(pl);
    view->setCurrentIndex(index);
    view->selectionModel()->clearSelection();
    view->selectionModel()->select(index, {QItemSelectionModel::Select});
    view->scrollToBottom();
    proxy->activeModel()->saveCurrentPlaylistIndex(index);
    emit loaded(pl);
    emit selected(pl);
    spinner->hide();
  }

  void Controller::on_search(const QString &term) {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    QRegularExpression regex(QRegularExpression::escape(term));
    proxy->setFilterRegularExpression(regex);
#else
    QRegExp regex(term, Qt::CaseInsensitive, QRegExp::Wildcard);
    proxy->setFilterRegExp(regex);
#endif
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

    view->setDragDropMode(term.isEmpty() ? QAbstractItemView::InternalMove : QAbstractItemView::NoDragDrop);

    /*QTimer::singleShot(20, [=]() {
      if (!view->selectionModel()->selectedRows().isEmpty()) {
        view->scrollTo(view->selectionModel()->selectedRows().first(), QAbstractItemView::PositionAtCenter);
      }
    });*/
  }
}
