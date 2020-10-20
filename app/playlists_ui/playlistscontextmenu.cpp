#include "playlistscontextmenu.h"

#include <QMenu>
#include <QAction>
#include <QDesktopServices>
#include <QClipboard>
#include <QApplication>
#include <QUrl>
#include <QInputDialog>
#include <QFileDialog>
#include <QStandardPaths>

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
    QAction remove(tr("Remove"));
    QAction rename(tr("Rename"));
    QAction savem3u(tr("Save as m3u"));

    connect(&remove, &QAction::triggered, [&]() {
      emit removed(index);
    });

    connect(&rename, &QAction::triggered, [&]() {
      auto i = model->itemAt(proxy->mapToSource(index));
      bool ok;
      QString new_name = QInputDialog::getText(view, QString("%1 '%2'").arg(tr("Rename playlist")).arg(i->name()), "", QLineEdit::Normal, i->name(), &ok, Qt::Widget);
      if (ok && !new_name.isEmpty()) {
        i->rename(new_name);
      }
    });
    QAction clear_filter(tr("Clear filter"));
    if (!search->text().isEmpty()) {
       connect(&clear_filter, &QAction::triggered, [=]() {
         search->clear();
       });
       menu.addAction(&clear_filter);
       menu.addSeparator();
    }

    connect(&savem3u, &QAction::triggered, [&]() {
      auto i = model->itemAt(proxy->mapToSource(index));
      QStringList m3u = i->toM3U();
      QString fname = QFileDialog::getSaveFileName(view, tr("Save as m3u"), QStandardPaths::writableLocation(QStandardPaths::MusicLocation) + "/" + i->name(), "m3u (*.m3u)");
      QFile f(fname);
      if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        f.write(m3u.join("\n").toUtf8());
        f.close();
      } else {
        qWarning() << "error opening file " << fname << "for writing";
      }
    });

    menu.addAction(&rename);
    menu.addAction(&remove);
    menu.addAction(&savem3u);
    menu.exec(view->viewport()->mapToGlobal(pos));
  }
}
