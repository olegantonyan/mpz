#include "playlistcontextmenu.h"

#include <QMenu>
#include <QAction>
#include <QDesktopServices>
#include <QClipboard>
#include <QApplication>
#include <QUrl>

namespace PlaylistUi {
  PlaylistContextMenu::PlaylistContextMenu(Model *m, ProxyFilterModel *p, QTableView *v, QLineEdit *s, QObject *parent) : QObject(parent), model(m), proxy(p), view(v), search(s) {
    Q_ASSERT(model);
    Q_ASSERT(proxy);
    Q_ASSERT(view);
    Q_ASSERT(search);
  }

  void PlaylistContextMenu::show(const QPoint &pos) {
    if(!view->indexAt(pos).isValid()) {
      return;
    }

    QMenu menu;
    QAction remove("Remove");
    connect(&remove, &QAction::triggered, [&]() {
      QList <QModelIndex> lst;
      for (auto i : view->selectionModel()->selectedRows()) {
        lst << proxy->mapToSource(i);
      }
      model->remove(lst);
      emit playlistChanged(model->playlist());
    });
    QAction open_in_filemanager("Show in file manager");
    connect(&open_in_filemanager, &QAction::triggered, [&]() {
      QStringList str;
      for (auto i : view->selectionModel()->selectedRows()) {
        auto dir = model->itemAt(proxy->mapToSource(i)).dir();
        if (!str.contains(dir)) {
          str << dir;
          QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
        }
      }
    });
    QAction copy_name("Copy name");
    connect(&copy_name, &QAction::triggered, [&]() {
      QStringList str;
      for (auto i : view->selectionModel()->selectedRows()) {
        str << model->itemAt(proxy->mapToSource(i)).formattedTitle();
      }
      qApp->clipboard()->setText(str.join('\n'));
    });
    QAction clear_filter("Clear filter");
    if (!search->text().isEmpty()) {
       connect(&clear_filter, &QAction::triggered, [&]() {
         search->clear();
       });
       menu.addAction(&clear_filter);
       menu.addSeparator();
    }

    /*QMenu move_to;
    move_to.setTitle("Move to playlist");
    move_to.addAction(&clear_filter);
    QMenu copy_to;
    copy_to.setTitle("Copy to playlist");
    copy_to.addAction(&clear_filter);

    menu.addMenu(&move_to);
    menu.addMenu(&copy_to);*/
    menu.addAction(&copy_name);
    menu.addAction(&open_in_filemanager);
    menu.addSeparator();
    menu.addAction(&remove);
    menu.exec(view->viewport()->mapToGlobal(pos));
  }
}
