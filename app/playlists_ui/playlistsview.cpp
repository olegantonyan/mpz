#include "playlistsview.h"
#include "playlist.h"

#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QInputDialog>

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
    //qDebug() << "add directory to playlist" << filepath;
    Playlist item(filepath);
    view->setCurrentIndex(model->append(item));
    emit selected(item);
  }

  void View::on_customContextMenuRequested(const QPoint &pos) {
    auto index = view->indexAt(pos);

    QMenu menu;
    QAction remove("Remove");
    QAction rename("Rename");

    connect(&remove, &QAction::triggered, [=]() { model->remove(index); });
    connect(&rename, &QAction::triggered, [=]() {
      Playlist i = model->itemAt(index);
      bool ok;
      QString new_name = QInputDialog::getText(view, QString("Rename playlist '%1'").arg(i.name()), "", QLineEdit::Normal, i.name(), &ok, Qt::Widget);
      if (ok && !new_name.isEmpty()) {
        i.rename(new_name);
        model->repalceAt(index, i);
      }
    });

    menu.addAction(&rename);
    menu.addAction(&remove);
    menu.exec(view->viewport()->mapToGlobal(pos));
  }

  void View::on_itemActivated(const QModelIndex &index) {
    auto item = model->itemAt(index);
    emit selected(item);
  }
}
