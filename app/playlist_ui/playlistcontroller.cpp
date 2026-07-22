#include "playlistcontroller.h"
#include "dropdirs.h"
#include "streamrowdelegate.h"

#include <QDebug>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QScrollBar>
#include <QThread>
#include <QTimer>
#include <QMouseEvent>
#include <QDropEvent>
#include <QMimeData>

namespace PlaylistUi {
  Controller::Controller(QTableView *v, QLineEdit *s, BusySpinner *sp, Config::Local &local_cfg, Config::Global &global_cfg,  ModusOperandi &modus, QObject *parent) : QObject(parent), search(s), spinner(sp), local_conf(local_cfg), global_conf(global_cfg), modus_operandi(modus) {
    restore_scroll_once = true;
    view = v;
    scroll_positions.clear();

    loadColumnsConfig();

    proxy = new ProxyFilterModel(view->style(), columns_config, modus, this);
    view->setModel(proxy);
    view->setItemDelegate(new StreamRowDelegate(this));
    view->horizontalHeader()->hide();
    view->verticalHeader()->hide();
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setSelectionMode(QAbstractItemView::ExtendedSelection);
    view->setDragEnabled(true);
    view->setAcceptDrops(true);
    view->setDropIndicatorShown(true);
    view->setDragDropMode(QAbstractItemView::InternalMove);
    view->setDefaultDropAction(Qt::MoveAction);
    view->setShowGrid(false);
    //view->setFocusPolicy(Qt::NoFocus);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    //view->horizontalHeader()->setStretchLastSection(true);

    view->setAlternatingRowColors(true);
    view->setFocus();

    auto row_height = view->verticalHeader()->minimumSectionSize(); //broken in Plasma 5.27
    view->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    if (global_conf.playlistRowHeight() != 0) {
      row_height = global_conf.playlistRowHeight();
    }
    view->verticalHeader()->setMinimumSectionSize(row_height);
    view->verticalHeader()->setDefaultSectionSize(row_height);

    for (int c = 0; c < view->horizontalHeader()->count(); ++c) {
      view->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Fixed);
    }

    view->viewport()->installEventFilter(this);
    view->installEventFilter(this);

    connect(view, &QTableView::activated, this, [=](const QModelIndex &index) {
      emit activated(proxy->activeModel()->itemAt(proxy->mapToSource(index)));
    });

    connect(view->selectionModel(), &QItemSelectionModel::currentChanged, this, &Controller::on_currentSelectionChanged);
    connect(view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &Controller::on_selectionChanged);

    connect(search, &QLineEdit::textChanged, this, &Controller::on_search);
    search->setClearButtonEnabled(true);

    connect(view->verticalScrollBar(), &QScrollBar::valueChanged, this, [=](int val) {
      if (proxy->activeModel()->playlist() != nullptr) {
        scroll_positions[proxy->activeModel()->playlist()->uid()] = val;
      }
    });

    context_menu = new PlaylistContextMenu(proxy, view, search, global_conf, this);
    connect(context_menu, &PlaylistContextMenu::playlistChanged, this, &Controller::changed);
    connect(context_menu, &PlaylistContextMenu::tracksChanged, this, &Controller::on_tracksChanged);

    view->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(view, &QTableView::customContextMenuRequested, context_menu, &PlaylistContextMenu::show);

    connect(proxy, &ProxyFilterModel::appendToPlaylistAsyncFinished, this, &Controller::on_appendAsyncFinished);

    connect(proxy, &ProxyFilterModel::tracksReordered, this, [this]() {
      if (persist_pending) {
        return;
      }
      persist_pending = true;
      QTimer::singleShot(0, this, [this]() {
        persist_pending = false;
        emit changed(proxy->activeModel()->playlist());
      });
    });

    connect(proxy, &QAbstractItemModel::modelReset, this, &Controller::updateStreamSpans);
    connect(proxy, &QAbstractItemModel::layoutChanged, this, &Controller::updateStreamSpans);
    connect(proxy, &QAbstractItemModel::rowsInserted, this, &Controller::updateStreamSpans);
    connect(proxy, &QAbstractItemModel::rowsRemoved, this, &Controller::updateStreamSpans);
    connect(proxy, &QAbstractItemModel::rowsMoved, this, &Controller::updateStreamSpans);
  }

  void Controller::updateStreamSpans() {
    view->clearSpans();
    const int cols = proxy->columnCount();
    if (cols <= 1) {
      return;
    }
    const int rows = proxy->rowCount();
    for (int r = 0; r < rows; r++) {
      if (proxy->index(r, 0).data(Model::IsStreamRole).toBool()) {
        view->setSpan(r, 1, 1, cols - 1);
      }
    }
  }

  void PlaylistUi::Controller::loadColumnsConfig() {
    auto c = global_conf.columnsConfig();
    if (c.count() == 0) {
      global_conf.saveColumnsConfig(columns_config);
    } else {
      columns_config = global_conf.columnsConfig();
    }
  }

  void Controller::on_load(const std::shared_ptr<Playlist::Playlist> pi) {
    if (pi == nullptr) {
      return;
    }
    proxy->activeModel()->setPlaylist(pi);

    if (scroll_positions.contains(pi->uid())) {
      QTimer::singleShot(20, this, [=]() { // hack: https://stackoverflow.com/questions/50989433/qtableviewscrollto-immediately-after-model-reset-and-after-some-delay
        view->verticalScrollBar()->setValue(scroll_positions[pi->uid()]);
      });
    }
  }

  void Controller::on_unload() {
    proxy->activeModel()->setPlaylist(nullptr);
  }

  void Controller::on_stop() {
    if (live_stream_uid != 0) {
      proxy->activeModel()->updateStreamMeta(live_stream_uid, StreamMetaData());
      live_stream_uid = 0;
    }
    proxy->activeModel()->highlight(0, Model::HighlightState::None);
  }

  void Controller::on_start(const Track &t) {
    if (live_stream_uid != 0 && live_stream_uid != t.uid()) {
      proxy->activeModel()->updateStreamMeta(live_stream_uid, StreamMetaData());
    }
    live_stream_uid = t.isStream() ? t.uid() : 0;
    proxy->activeModel()->highlight(t.uid(), Model::HighlightState::Playing);
  }

  void Controller::on_trackMetaChanged(const Track &t) {
    if (!t.isStream()) {
      return;
    }
    live_stream_uid = t.uid();
    proxy->activeModel()->updateStreamMeta(t.uid(), t.streamMeta());
  }

  void Controller::on_pause(const Track &t) {
    proxy->activeModel()->highlight(t.uid(), Model::HighlightState::Paused);
  }

  void Controller::on_scrollTo(const Track &track) {
    QModelIndex index = proxy->mapFromSource(proxy->activeModel()->indexOf(track.uid()));
    if (index.isValid()) {
      view->setCurrentIndex(index);
      view->scrollTo(index, QAbstractItemView::PositionAtCenter);
      emit selected(track);
    }
  }

  void Controller::on_appendToPlaylist(const QList<QDir> &filepaths) {
    if (proxy->activeModel()->playlist() != nullptr) {
      proxy->activeModel()->appendToPlaylistAsync(filepaths);
      spinner->show();
    }
  }

  void Controller::on_appendTracks(const QVector<Track> &tracks) {
    if (proxy->activeModel()->playlist() != nullptr) {
      proxy->activeModel()->appendTracks(tracks);
    }
  }

  void Controller::sortBy(const QString &criteria) {
    if (proxy->activeModel()->playlist() != nullptr) {
      proxy->activeModel()->sortBy(criteria);
      emit changed(proxy->activeModel()->playlist());
    }
  }

  void Controller::on_appendAsyncFinished(std::shared_ptr<Playlist::Playlist> pl) {
    Q_ASSERT(pl == proxy->activeModel()->playlist());

    proxy->activeModel()->reload();
    emit changed(proxy->activeModel()->playlist());
    spinner->hide();
  }

  void Controller::on_tracksChanged(const std::shared_ptr<Playlist::Playlist> pl, const QList<quint64> &uids) {
    if (pl == nullptr || uids.isEmpty()) {
      return;
    }
    for (quint64 uid : uids) {
      pl->reloadTrack(uid);
    }
    if (pl == proxy->activeModel()->playlist()) {
      proxy->activeModel()->reload();
    }
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

    auto model = proxy->activeModel();
    if (model->playlist() == nullptr) {
      emit createPlaylistRequested(dirs, DropUtil::commonParentDir(dirs));
      return;
    }

    const QPoint pos = DropUtil::dropPosition(event);
    const auto index = view->indexAt(pos);
    int at_row;
    if (!index.isValid()) {
      at_row = model->rowCount();
    } else {
      const QRect rect = view->visualRect(index);
      const bool below = pos.y() > rect.center().y();
      at_row = proxy->mapToSource(index).row() + (below ? 1 : 0);
    }

    model->insertTracksAsync(dirs, at_row);
    spinner->show();
  }

  void Controller::eventFilterTableView(QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
      QKeyEvent* keyevent = dynamic_cast<QKeyEvent*>(event);
      if (keyevent->key() == Qt::Key_Delete
#ifdef Q_OS_MACOS
          || keyevent->key() == Qt::Key_Backspace
#endif
         ) {
        context_menu->on_remove();
      } else if (keyevent->key() == Qt::Key_I && keyevent->modifiers().testFlag(Qt::ControlModifier)) {
        context_menu->on_trackInfo();
      }
    }
  }

  void Controller::eventFilterViewport(QEvent *event) {
    if (event->type() == QEvent::Resize) {
      int total_width = view->width();
      view->horizontalHeader()->setMinimumSectionSize(20);
      view->setColumnWidth(0, 20);

      for (int col = 1; col <= columns_config.count(); col++) {
        auto rel_width = columns_config.width(col);
        if (rel_width > 0) {
          view->setColumnWidth(col, static_cast<int>(total_width * rel_width));
        }
        if (columns_config.stretch(col)) {
          view->horizontalHeader()->setSectionResizeMode(col, QHeaderView::Stretch);
        }
      }

    } else if (event->type() == QEvent::WindowActivate) {
      if (restore_scroll_once) {
        restore_scroll_once = false;
        view->verticalScrollBar()->setValue(local_conf.playlistViewScrollPosition());
      }
    } else if (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress) {
      local_conf.savePlaylistViewScrollPosition(view->verticalScrollBar()->value());
    }

    if (event->type() == QEvent::MouseButtonPress) {
      QMouseEvent *me = dynamic_cast<QMouseEvent *>(event);
      if (me->button() == Qt::BackButton) {
        search->clear();
      }
      if (me->button() == Qt::LeftButton) {
        auto idx = view->indexAt(me->pos());
        view->setDragEnabled(idx.isValid() && view->selectionModel()->isSelected(idx));
      }
    }

    if (event->type() == QEvent::MouseButtonRelease) {
      QMouseEvent *me = dynamic_cast<QMouseEvent *>(event);
      if (me->button() == Qt::LeftButton) {
        view->setDragEnabled(true);
      }
    }
  }

  void Controller::on_search(const QString &term) {
    proxy->filter(term);
    view->setDragDropMode(term.isEmpty() ? QAbstractItemView::InternalMove : QAbstractItemView::NoDragDrop);
    if (term.isEmpty()) {
      QTimer::singleShot(20, this, [=]() {
        if (!view->selectionModel()->selectedRows().isEmpty()) {
          view->scrollTo(view->selectionModel()->selectedRows().first(), QAbstractItemView::PositionAtCenter);
        }
      });
    }
  }

  void Controller::on_currentSelectionChanged(const QModelIndex &index, const QModelIndex &prev) {
    Q_UNUSED(prev)
    auto source_index = proxy->mapToSource(index);
    if (index.isValid() && source_index.isValid() && source_index.row() < proxy->activeModel()->rowCount()) {
      emit selected(proxy->activeModel()->itemAt(source_index));
    }
  }

  void Controller::on_selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
    Q_UNUSED(deselected)
    Q_UNUSED(selected)

    quint32 selection_time = 0;
    for (const auto &i : view->selectionModel()->selectedRows()) {
      auto source_index = proxy->mapToSource(i);
      if (i.isValid() && source_index.isValid() && source_index.row() < proxy->activeModel()->rowCount()) {
        selection_time += proxy->activeModel()->itemAt(source_index).duration();
      }
    }
    emit durationOfSelectedChanged(selection_time);
  }
}
