#include "playlistcontroller.h"
#include "dropdirs.h"
#include "streamrowdelegate.h"
#include "tracksmimedata.h"

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

    view->viewport()->setMouseTracking(true);
    setupFloatingHeader();

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
    connect(context_menu, &PlaylistContextMenu::removeRequested, this, &Controller::removeSelectedTracks);
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

  void Controller::setupFloatingHeader() {
    QStringList labels;
    QVector<Qt::Alignment> aligns;
    labels << QString();
    aligns << (Qt::AlignLeft | Qt::AlignVCenter);
    for (int col = 1; col <= columns_config.count(); col++) {
      labels << columns_config.field(col);
      aligns << columns_config.align(col);
    }

    floating_header = new FloatingHeader(view);
    floating_header->setColumns(labels, aligns);
    floating_header->setActive(global_conf.playlistHeaderEnabled());

    auto *header = view->horizontalHeader();
    connect(header, &QHeaderView::sectionResized, floating_header, &FloatingHeader::invalidateCache);
    connect(header, &QHeaderView::geometriesChanged, floating_header, &FloatingHeader::invalidateCache);
    connect(header, &QHeaderView::sectionCountChanged, floating_header, &FloatingHeader::syncGeometry);
    connect(proxy, &QAbstractItemModel::modelReset, floating_header, &FloatingHeader::syncGeometry);
    connect(view->horizontalScrollBar(), &QScrollBar::valueChanged, floating_header, &FloatingHeader::invalidateCache);
    connect(view, &QTableView::customContextMenuRequested, floating_header, &FloatingHeader::hideNow);
  }

  void Controller::setFloatingHeaderEnabled(bool enabled) {
    floating_header->setActive(enabled);
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

  void Controller::on_removeTracks(quint64 playlist_uid, const QVector<Track> &tracks) {
    auto model = proxy->activeModel();
    if (model->playlist() == nullptr || model->playlist()->uid() != playlist_uid) {
      return;
    }
    QList<quint64> wanted;
    for (const auto &track : tracks) {
      wanted << track.uid();
    }
    QList<QModelIndex> items;
    for (int row = 0; row < model->tracksSize(); row++) {
      const int at = wanted.indexOf(model->trackAt(row).uid());
      if (at >= 0) {
        wanted.removeAt(at);
        items << model->buildIndex(row);
      }
    }
    if (items.isEmpty()) {
      return;
    }
    model->remove(items);
    emit changed(model->playlist());
  }

  void Controller::on_tracksAppended(const std::shared_ptr<Playlist::Playlist> pl) {
    if (pl != nullptr && pl == proxy->activeModel()->playlist()) {
      proxy->activeModel()->reload();
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
      floating_header->onViewportEvent(event);
      if (handleDnd(event)) {
        return true;
      }
      eventFilterViewport(event);
    } else if (obj == view) {
      floating_header->onViewEvent(event);
      eventFilterTableView(event);
    }
    return QObject::eventFilter(obj, event);
  }

  const TracksMimeData *Controller::droppedTracks(QDropEvent *event) const {
    if (event->source() == view) { // an internal reorder, not a cross-view drop
      return nullptr;
    }
    return TracksMimeData::from(event->mimeData());
  }

  bool Controller::handleDnd(QEvent *event) {
    const auto type = event->type();
    if (type != QEvent::DragEnter && type != QEvent::DragMove && type != QEvent::Drop) {
      return false;
    }
    if (modus_operandi.get() != ModusOperandi::MODUS_LOCALFS) {
      return false;
    }
    auto *drop_event = static_cast<QDropEvent *>(event);
    const bool tracks = droppedTracks(drop_event) != nullptr;
    if (!tracks && !drop_event->mimeData()->hasUrls()) {
      return false;
    }
    if (type == QEvent::Drop) {
      onDrop(drop_event);
    }
    if (tracks) {
      drop_event->setDropAction(Qt::CopyAction);
      drop_event->accept();
    } else {
      drop_event->acceptProposedAction();
    }
    return true;
  }

  void Controller::onDrop(QDropEvent *event) {
    const auto *tracks_mime = droppedTracks(event);
    const auto dirs = tracks_mime != nullptr ? QList<QDir>() : DropUtil::droppedDirs(event->mimeData());
    if (tracks_mime == nullptr && dirs.isEmpty()) {
      return;
    }

    auto model = proxy->activeModel();
    if (model->playlist() == nullptr) {
      if (tracks_mime != nullptr) {
        emit createPlaylistFromTracksRequested(tracks_mime->tracks(), tracks_mime->suggestedName());
      } else {
        emit createPlaylistRequested(dirs, DropUtil::libraryRoot(event->mimeData(), dirs));
      }
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

    if (tracks_mime != nullptr) {
      model->insertTracks(tracks_mime->tracks(), at_row);
      return;
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
        removeSelectedTracks();
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
      floating_header->syncGeometry();

    } else if (event->type() == QEvent::WindowActivate) {
      if (restore_scroll_once) {
        restore_scroll_once = false;
        view->verticalScrollBar()->setValue(local_conf.playlistViewScrollPosition());
      }
    } else if (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress) {
      const int scroll = view->verticalScrollBar()->value();
      if (scroll != last_saved_scroll) {
        last_saved_scroll = scroll;
        local_conf.savePlaylistViewScrollPosition(scroll);
      }
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

  void Controller::removeSelectedTracks() {
    const auto rows = view->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
      return;
    }

    QList<QModelIndex> source_rows;
    int fallback_row = proxy->rowCount();
    for (const auto &i : rows) {
      source_rows << proxy->mapToSource(i);
      fallback_row = qMin(fallback_row, i.row());
    }
    const quint64 cursor_uid = proxy->activeModel()->itemAt(proxy->mapToSource(view->currentIndex())).uid();

    proxy->activeModel()->remove(source_rows);
    emit changed(proxy->activeModel()->playlist());
    restoreCursor(cursor_uid, fallback_row);
  }

  // removal rebuilds the model and drops the cursor; left invalid, Qt moves it to the first row on
  // the next focus-in, which would hijack "playback follows cursor"
  void Controller::restoreCursor(quint64 cursor_uid, int fallback_row) {
    if (proxy->rowCount() == 0) {
      return;
    }

    QModelIndex index = proxy->mapFromSource(proxy->activeModel()->indexOf(cursor_uid));
    const bool cursor_survived = index.isValid();
    if (!cursor_survived) {
      index = proxy->index(qBound(0, fallback_row, proxy->rowCount() - 1), 0);
      if (!index.isValid()) {
        return;
      }
    }

    restoring_cursor = true;
    view->setCurrentIndex(index);
    restoring_cursor = false;

    if (!cursor_survived) {
      emit cursorRestored(proxy->activeModel()->itemAt(proxy->mapToSource(index)));
    }
  }

  void Controller::on_currentSelectionChanged(const QModelIndex &index, const QModelIndex &prev) {
    Q_UNUSED(prev)
    if (restoring_cursor) {
      return;
    }
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
