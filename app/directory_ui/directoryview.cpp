#include "directoryview.h"

#include <QAction>
#include <QDebug>
#include <QMenu>
#include <QHeaderView>
#include <QMouseEvent>
#include <QScrollBar>

namespace DirectoryUi {
  View::View(QTreeView *v, Config::Local &local_cfg, QObject *parent) : QObject(parent), view(v), local_conf(local_cfg) {

    QString path;
    if (local_conf.libraryPaths().empty()) {
      path = QDir::homePath();
    } else {
      path = local_conf.libraryPaths().first();
    }

    model = new Model(path, this);

    view->setModel(model);
    view->setRootIndex(model->index(path));
    view->setHeaderHidden(true);
    view->setColumnHidden(1, true);
    view->setColumnHidden(2, true);
    view->setColumnHidden(3, true);
    view->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(view, &QTreeView::customContextMenuRequested, this, &View::on_customContextMenuRequested);

    view->viewport()->installEventFilter(this);
  }

  void View::on_customContextMenuRequested(const QPoint &pos) {
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

    //qDebug() << view->verticalScrollBar()->value();
  }

  bool View::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress) {
      QMouseEvent *me = dynamic_cast<QMouseEvent *>(event);
      if (me->button() == Qt::MidButton) {
        auto index = view->indexAt(me->pos());
        if (index.isValid()) {
          auto filepath = QDir(model->filePath(index));
          emit createNewPlaylist(filepath);
        }
      }
    }
    return QObject::eventFilter(obj, event);
  }
}
