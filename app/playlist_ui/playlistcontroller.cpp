#include "playlistcontroller.h"

#include <QDebug>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QScrollBar>
#include <QThread>
#include <QTimer>
#include <QMouseEvent>

namespace PlaylistUi {    
  Controller::Controller(QTableView *v, QLineEdit *s, BusySpinner *sp, Config::Local &local_cfg, Config::Global &global_cfg, QObject *parent) : QObject(parent), search(s), spinner(sp), local_conf(local_cfg), global_conf(global_cfg) {
    restore_scroll_once = true;
    view = v;
    scroll_positions.clear();

    loadColumnsConfig();

    model = new Model(view->style(), columns_config, this);
    proxy = new ProxyFilterModel(model, this);
    view->setModel(proxy);
    view->horizontalHeader()->hide();
    view->verticalHeader()->hide();
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
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

    connect(view, &QTableView::activated, [=](const QModelIndex &index) {
      emit activated(model->itemAt(proxy->mapToSource(index)));
    });

    connect(view->selectionModel(), &QItemSelectionModel::currentChanged, this, &Controller::on_currentSelectionChanged);
    connect(view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &Controller::on_selectionChanged);

    connect(search, &QLineEdit::textChanged, this, &Controller::on_search);
    search->setClearButtonEnabled(true);

    connect(view->verticalScrollBar(), &QScrollBar::valueChanged, [=](int val) {
      if (model->playlist() != nullptr) {
        scroll_positions[model->playlist()->uid()] = val;
      }
    });

    context_menu = new PlaylistContextMenu(model, proxy, view, search, this);
    connect(context_menu, &PlaylistContextMenu::playlistChanged, this, &Controller::changed);

    view->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(view, &QTableView::customContextMenuRequested, context_menu, &PlaylistContextMenu::show);
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
    model->setPlaylist(pi);

    if (scroll_positions.contains(pi->uid())) {
      QTimer::singleShot(20, [=]() { // hack: https://stackoverflow.com/questions/50989433/qtableviewscrollto-immediately-after-model-reset-and-after-some-delay
        view->verticalScrollBar()->setValue(scroll_positions[pi->uid()]);
      });
    }
  }

  void Controller::on_unload() {
    model->setPlaylist(nullptr);
  }

  void Controller::on_stop() {
    model->highlight(0, Model::HighlightState::None);
  }

  void Controller::on_start(const Track &t) {
    model->highlight(t.uid(), Model::HighlightState::Playing);
  }

  void Controller::on_pause(const Track &t) {
    model->highlight(t.uid(), Model::HighlightState::Paused);
  }

  void Controller::on_scrollTo(const Track &track) {
    QModelIndex index = proxy->mapFromSource(model->indexOf(track.uid()));
    if (index.isValid()) {
      view->setCurrentIndex(index);
      view->scrollTo(index, QAbstractItemView::PositionAtCenter);
      emit selected(track);
    }
  }

  void Controller::on_appendToPlaylist(const QList<QDir> &filepaths) {
    if (model->playlist() != nullptr) {
      connect(&*model->playlist(), &Playlist::Playlist::concatAsyncFinished, this, &Controller::on_appendAsyncFinished);
      model->playlist()->concatAsync(filepaths);
      spinner->show();
    }
  }

  void Controller::sortBy(const QString &criteria) {
    if (model->playlist() != nullptr) {
      model->playlist()->sortBy(criteria);
      model->reload();
      emit changed(model->playlist());
    }
  }

  void Controller::on_appendAsyncFinished(Playlist::Playlist *pl) {
    disconnect(pl, &Playlist::Playlist::concatAsyncFinished, this, &Controller::on_appendAsyncFinished);
    model->reload();
    emit changed(model->playlist());
    spinner->hide();
  }

  bool Controller::eventFilter(QObject *obj, QEvent *event) {
    if (obj == view->viewport()) {
      eventFilterViewport(event);
    } else if (obj == view) {
      eventFilterTableView(event);
    }
    return QObject::eventFilter(obj, event);
  }

  void Controller::eventFilterTableView(QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
      QKeyEvent* keyevent = dynamic_cast<QKeyEvent*>(event);
      if (keyevent->key() == Qt::Key_Delete) {
        context_menu->on_remove();
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
    }
  }

  void Controller::on_search(const QString &term) {
    proxy->filter(term);
    if (term.isEmpty()) {
      QTimer::singleShot(20, [=]() {
        if (!view->selectionModel()->selectedRows().isEmpty()) {
          view->scrollTo(view->selectionModel()->selectedRows().first(), QAbstractItemView::PositionAtCenter);
        }
      });
    }
  }

  void Controller::on_currentSelectionChanged(const QModelIndex &index, const QModelIndex &prev) {
    Q_UNUSED(prev)
    auto source_index = proxy->mapToSource(index);
    if (index.isValid() && source_index.isValid() && source_index.row() < model->rowCount()) {
      emit selected(model->itemAt(source_index));
    }
  }

  void Controller::on_selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
    Q_UNUSED(deselected)
    Q_UNUSED(selected)

    quint32 selection_time = 0;
    for (auto i: view->selectionModel()->selectedRows()) {
      auto source_index = proxy->mapToSource(i);
      if (i.isValid() && source_index.isValid() && source_index.row() < model->rowCount()) {
        selection_time += model->itemAt(source_index).duration();
      }
    }
    emit durationOfSelectedChanged(selection_time);
  }
}
