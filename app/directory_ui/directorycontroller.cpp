#include "directorycontroller.h"

#include <QAction>
#include <QDebug>
#include <QMenu>
#include <QHeaderView>
#include <QMouseEvent>
#include <QScrollBar>
#include <iostream>

namespace DirectoryUi {
  Controller::Controller(QTreeView *v, QLineEdit *search, QComboBox *libswitch, Config::Local &local_cfg, QObject *parent) :
    QObject(parent),
    view(v),
    local_conf(local_cfg) {

    restore_scroll_once = true;

    model = new Model(this);
    if (local_conf.libraryPaths().empty()) {
      model->loadAsync(QDir::homePath());
      libswitch->addItem(QDir::homePath());
    } else {
      for (auto i : local_conf.libraryPaths()) {
        libswitch->addItem(i);
      }
      model->loadAsync(local_conf.libraryPaths().first());
    }

    connect(libswitch, QOverload<int>::of(&QComboBox::activated), [=](int idx) {
      model->loadAsync(local_conf.libraryPaths()[idx]);
    });

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
  }

  void Controller::on_customContextMenuRequested(const QPoint &pos) {
    auto index = view->indexAt(pos);
    if (!index.isValid()) {
      return;
    }

    auto filepath = QDir(model->filePath(index));

    QMenu menu;
    QAction create_playlist("Create new playlist");
    QAction append_to_playlist("Append to current playlist");

    connect(&create_playlist, &QAction::triggered, [=]() {
      emit createNewPlaylist(filepath);
    });
    connect(&append_to_playlist, &QAction::triggered, [=]() {
      emit appendToCurrentPlaylist(filepath);
    });

    menu.addAction(&create_playlist);
    menu.addAction(&append_to_playlist);
    menu.exec(view->viewport()->mapToGlobal(pos));
  }

  void Controller::on_search(const QString &term) {
    if (term.isEmpty()) {
      model->setNameFilters(QStringList());
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
      local_conf.saveLibraryViewScrollPosition(view->verticalScrollBar()->value());
    } else if (event->type() == QEvent::WindowActivate) {
      if (restore_scroll_once) {
        restore_scroll_once = false;
        view->verticalScrollBar()->setValue(local_conf.libraryViewScrollPosition());
      }
    }

    return QObject::eventFilter(obj, event);
  }
}
