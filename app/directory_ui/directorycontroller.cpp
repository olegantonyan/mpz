#include "directorycontroller.h"
#include "directorysettings.h"
#include "directorysortmenu.h"

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
#include <QtGlobal>
#include <QFileInfo>

namespace DirectoryUi {
  Controller::Controller(QTreeView *v, QLineEdit *s, QComboBox *libswitch, QToolButton *libcfg, QToolButton *libsort, Config::Local &local_cfg, QObject *parent) :
    QObject(parent),
    view(v),
    search(s),
    local_conf(local_cfg) {
    Q_ASSERT(search);
    Q_ASSERT(view);

    view->setSelectionMode(QTreeView::ExtendedSelection);

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

    context_menu = new DirectoryContextMenu(model, view, search, this);
    connect(context_menu, &DirectoryContextMenu::createNewPlaylist, this, &Controller::createNewPlaylist);
    connect(context_menu, &DirectoryContextMenu::appendToCurrentPlaylist, this, &Controller::appendToCurrentPlaylist);
    connect(view, &QTreeView::customContextMenuRequested, context_menu, &DirectoryContextMenu::show);

    view->viewport()->installEventFilter(this); // viewport for mouse events, doesn't work otherwise

    connect(view->verticalScrollBar(), &QScrollBar::valueChanged, [=](int val) {
      local_conf.saveLibraryViewScrollPosition(val);
    });

    connect(libcfg, &QToolButton::clicked, [=]() {
      settingsDialog(libswitch);
    });

    connect(view, &QTreeView::doubleClicked, this, &Controller::on_doubleclick);

    sort_menu = new DirectoryUi::SortMenu(libsort);
    connect(sort_menu, &DirectoryUi::SortMenu::triggered, model, &DirectoryUi::Model::sortBy);
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

  void Controller::on_doubleclick(const QModelIndex &index) {
    if (!index.isValid()) {
      return;
    }
    auto filepath = model->filePath(index);
    QFileInfo fi(filepath);
    if (fi.exists() && fi.isFile()) {
      emit createNewPlaylist({ QDir(filepath) });
    }
  }

  bool Controller::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress) {
      QMouseEvent *me = dynamic_cast<QMouseEvent *>(event);
      if (me->button() == Qt::MiddleButton) {
        auto index = view->indexAt(me->pos());
        if (index.isValid()) {
          auto filepath = QDir(model->filePath(index));
          emit createNewPlaylist({filepath});
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
