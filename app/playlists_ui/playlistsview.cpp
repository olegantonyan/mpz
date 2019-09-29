#include "playlistsview.h"
#include "playlist.h"

#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QItemSelectionModel>

namespace PlaylistsUi {
  View::View(QListView *v, QObject *parent) : QObject(parent) {
    view = v;
    model = new Model(this);

    view->setModel(model);
    view->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(view, &QListView::customContextMenuRequested, this, &View::on_customContextMenuRequested);
    connect(view, &QListView::clicked, this, &View::on_itemActivated);
  }

  void View::on_createPlaylist(const QDir &filepath) {
    auto item = std::shared_ptr<Playlist>(new Playlist(filepath));
    auto index = model->append(item);
    view->setCurrentIndex(index);
    view->selectionModel()->select(index, {QItemSelectionModel::Select});
    current = item;

    emit selected(item);
  }

  void View::on_customContextMenuRequested(const QPoint &pos) {
    auto index = view->indexAt(pos);
    if (!index.isValid()) {
      return;
    }

    QMenu menu;
    QAction remove("Remove");
    QAction rename("Rename");

    connect(&remove, &QAction::triggered, [=]() {
      model->remove(index);
      for (auto i : view->selectionModel()->selectedIndexes()) {
        if (i == index) {
          auto new_idx = model->index(qMax(index.row() - 1, 0));
          view->selectionModel()->select(new_idx, {QItemSelectionModel::Select});
          if (model->listSize() > 0) {
            on_itemActivated(new_idx);
          } else {
            emit emptied();
          }
        }
      }
    });
    connect(&rename, &QAction::triggered, [=]() {
      auto i = model->itemAt(index);
      bool ok;
      QString new_name = QInputDialog::getText(view,
                                               QString("Rename playlist '%1'").arg(i->name()),
                                               "",
                                               QLineEdit::Normal,
                                               i->name(),
                                               &ok,
                                               Qt::Widget);
      if (ok && !new_name.isEmpty()) {
        i->rename(new_name);
      }
    });

    menu.addAction(&rename);
    menu.addAction(&remove);
    menu.exec(view->viewport()->mapToGlobal(pos));
  }

  void View::on_itemActivated(const QModelIndex &index) {
    if (model->listSize() <= 0) {
      return;
    }

    auto item = model->itemAt(index);
    if (current != item) {
      emit selected(item);
    }
    current = item;
  }
}
