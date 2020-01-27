#include "playlistscontextmenu.h"

#include <QMenu>
#include <QAction>
#include <QDesktopServices>
#include <QClipboard>
#include <QApplication>
#include <QUrl>
#include <QInputDialog>

namespace PlaylistsUi {
  PlaylistsContextMenu::PlaylistsContextMenu(Model *m, ProxyFilterModel *p, QListView *v, QLineEdit *s, QObject *parent) : QObject(parent), model(m), proxy(p), view(v), search(s) {
    Q_ASSERT(model);
    Q_ASSERT(proxy);
    Q_ASSERT(view);
    Q_ASSERT(search);
  }

  void PlaylistsContextMenu::show(const QPoint &pos) {
    auto index = view->indexAt(pos);
    if (!index.isValid()) {
      return;
    }

    QMenu menu;
    QAction remove("Remove");
    QAction rename("Rename");

    connect(&remove, &QAction::triggered, [&]() {
      emit removed(index);
    });

    connect(&rename, &QAction::triggered, [&]() {
      auto i = model->itemAt(proxy->mapToSource(index));
      bool ok;
      QString new_name = QInputDialog::getText(view, QString("Rename playlist '%1'").arg(i->name()), "", QLineEdit::Normal, i->name(), &ok, Qt::Widget);
      if (ok && !new_name.isEmpty()) {
        i->rename(new_name);
      }
    });
    QAction clear_filter("Clear filter");
    if (!search->text().isEmpty()) {
       connect(&clear_filter, &QAction::triggered, [=]() {
         search->clear();
       });
       menu.addAction(&clear_filter);
       menu.addSeparator();
    }

    menu.addAction(&rename);
    menu.addAction(&remove);
    menu.exec(view->viewport()->mapToGlobal(pos));
  }
}
