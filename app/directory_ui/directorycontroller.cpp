#include "directorycontroller.h"
#include "directorysettings.h"

#include <QAction>
#include <QDebug>
#include <QMenu>
#include <QHeaderView>
#include <QMouseEvent>
#include <QScrollBar>
#include <QDesktopServices>
#include <QApplication>
#include <QUrl>
#include <QTimer>

namespace DirectoryUi {
  Controller::Controller(QTreeView *v, QLineEdit *s, QComboBox *libswitch, QToolButton *libcfg, Config::Local &local_cfg, QObject *parent) :
    QObject(parent),
    view(v),
    search(s),
    local_conf(local_cfg) {
    Q_ASSERT(search);
    Q_ASSERT(view);

    restore_scroll_once = true;

    model = new Model(this);
    if (local_conf.libraryPaths().empty()) {
      model->loadAsync(QDir::homePath());
      libswitch->addItem(QDir::homePath());
    } else {
      for (auto i : local_conf.libraryPaths()) {
        libswitch->addItem(i);
      }
      int current_index = qBound(0, local_conf.currentLibraryPath(), libswitch->count());
      libswitch->setCurrentIndex(current_index);
      model->loadAsync(local_conf.libraryPaths().at(current_index));
    }

    if (libswitch->count() > 1) {
      connect(libswitch, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int idx) {
        if (idx >= 0) {
          model->loadAsync(local_conf.libraryPaths()[idx]);
          local_conf.saveCurrentLibraryPath(idx);
        }
      });
    }

    connect(model, &DirectoryUi::Model::directoryLoaded, [=] {
      view->setModel(model);
      view->setRootIndex(model->index(model->rootPath()));
      view->setHeaderHidden(true);
      view->setColumnHidden(1, true);
      view->setColumnHidden(2, true);
      view->setColumnHidden(3, true);
      view->setContextMenuPolicy(Qt::CustomContextMenu);
    });

    model->setNameFilterDisables(false);
    model->setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    connect(search, &QLineEdit::textChanged, this, &DirectoryUi::Controller::on_search);

    search->setClearButtonEnabled(true);

    connect(view, &QTreeView::customContextMenuRequested, this, &Controller::on_customContextMenuRequested);

    view->viewport()->installEventFilter(this); // viewport for mouse events, doesn't work otherwise

    connect(view->verticalScrollBar(), &QScrollBar::valueChanged, [=](int val) {
      local_conf.saveLibraryViewScrollPosition(val);
    });

    connect(libcfg, &QToolButton::clicked, [=]() {
      settingsDialog(libswitch);
    });
  }

  void Controller::on_customContextMenuRequested(const QPoint &pos) {
    auto index = view->indexAt(pos);
    if (!index.isValid()) {
      return;
    }

    auto filepath = QDir(model->filePath(index));

    QMenu menu;
    QAction clear_filter("Clear filter");
    QAction create_playlist("Create new playlist");
    QAction append_to_playlist("Append to current playlist");
    QAction open_in_filemanager("Open in file manager");

    connect(&create_playlist, &QAction::triggered, [=]() {
      emit createNewPlaylist(filepath);
    });
    connect(&append_to_playlist, &QAction::triggered, [=]() {
      emit appendToCurrentPlaylist(filepath);
    });
    connect(&open_in_filemanager, &QAction::triggered, [=]() {
      QDesktopServices::openUrl(QUrl::fromLocalFile(filepath.absolutePath()));
    });
    if (!search->text().isEmpty()) {
      connect(&clear_filter, &QAction::triggered, [=]() {
        search->clear();
      });
      menu.addAction(&clear_filter);
      menu.addSeparator();
    }

    menu.addAction(&create_playlist);
    menu.addAction(&append_to_playlist);
    menu.addAction(&open_in_filemanager);
    menu.exec(view->viewport()->mapToGlobal(pos));
  }

  void Controller::on_search(const QString &term) {
    if (term.isEmpty()) {
      model->setNameFilters(QStringList());
      QTimer::singleShot(20, [=]() {
        if (!view->selectionModel()->selectedRows().isEmpty()) {
          view->scrollTo(view->selectionModel()->selectedRows().first(), QAbstractItemView::PositionAtCenter);
        }
      });
      return;
    }
    QString wc = QString("*%1*").arg(term);
    model->setNameFilters(QStringList() << wc);
  }

  bool Controller::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress) {
      QMouseEvent *me = dynamic_cast<QMouseEvent *>(event);
      if (me->button() == Qt::MidButton) {
        auto index = view->indexAt(me->pos());
        if (index.isValid()) {
          auto filepath = QDir(model->filePath(index));
          emit createNewPlaylist(filepath);
        }
      }
      if (me->button() == Qt::BackButton) {
        search->clear();
      }
    } else if (event->type() == QEvent::WindowActivate) {
      if (restore_scroll_once) {
        restore_scroll_once = false;
        view->verticalScrollBar()->setValue(local_conf.libraryViewScrollPosition());
      }
    }

    return QObject::eventFilter(obj, event);
  }

  void Controller::settingsDialog(QComboBox *libswitch) {
    DirectorySettings dlg(local_conf.libraryPaths());
    auto old_paths = local_conf.libraryPaths();
    if(dlg.exec() == QDialog::Accepted) {
      if (old_paths != dlg.libraryPaths()) {
        local_conf.saveLibraryPaths(dlg.libraryPaths());
        local_conf.sync();
        libswitch->clear();
        for (auto i : local_conf.libraryPaths()) {
          libswitch->addItem(i);
        }
        if (libswitch->count() > 0) {
          model->loadAsync(local_conf.libraryPaths()[libswitch->count() - 1]);
          libswitch->setCurrentIndex(libswitch->count() - 1);
        }
      }
    }
  }
}
