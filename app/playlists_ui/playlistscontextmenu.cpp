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
    QAction reload(tr("Reload from filesystem"));
    QAction play(tr("Play"));

    connect(&reload, &QAction::triggered, [&]() {
      on_reload(index);
    });
    reload.setIcon(view->style()->standardIcon(QStyle::SP_BrowserReload));

    remove.setIcon(view->style()->standardIcon(QStyle::SP_TrashIcon));
    savem3u.setIcon(view->style()->standardIcon(QStyle::SP_DialogSaveButton));
    //rename.setIcon(view->style()->standardIcon(QStyle::SP_FileIcon));
    play.setIcon(view->style()->standardIcon(QStyle::SP_MediaPlay));

    connect(&remove, &QAction::triggered, [&]() {
      emit removed(index);
    });

    connect(&rename, &QAction::triggered, [&]() {
      on_rename(index);
    });

    QAction clear_filter(tr("Clear filter"));
    clear_filter.setIcon(view->style()->standardIcon(QStyle::SP_DialogCancelButton));
    if (!search->text().isEmpty()) {
       connect(&clear_filter, &QAction::triggered, [=]() {
         search->clear();
       });
       menu.addAction(&clear_filter);
       menu.addSeparator();
    }

    connect(&savem3u, &QAction::triggered, [&]() {
      on_savem3u(index);
    });

    connect(&play, &QAction::triggered, [&]() {
      emit view->doubleClicked(index);
    });

    menu.addAction(&play);
    menu.addSeparator();
    menu.addAction(&rename);
    menu.addAction(&savem3u);
    menu.addAction(&reload);
    menu.addSeparator();
    menu.addAction(&remove);
    menu.exec(view->viewport()->mapToGlobal(pos));
  }

  void PlaylistsContextMenu::on_rename(const QModelIndex &index)  {
    auto i = model->itemAt(proxy->mapToSource(index));
    bool ok;
    QString new_name = QInputDialog::getText(view, QString("%1 '%2'").arg(tr("Rename playlist")).arg(i->name()), "", QLineEdit::Normal, i->name(), &ok, Qt::Widget);
    if (ok && !new_name.isEmpty()) {
      i->rename(new_name);
    }
    model->persist();
  }

  void PlaylistsContextMenu::on_savem3u(const QModelIndex &index) {
    auto i = model->itemAt(proxy->mapToSource(index));
    QByteArray m3u = i->toM3U();
    QString fname = QFileDialog::getSaveFileName(view, tr("Save as m3u"), QStandardPaths::writableLocation(QStandardPaths::MusicLocation) + "/" + i->name(), "m3u (*.m3u)");
    QFile f(fname);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
      f.write(m3u);
      f.close();
    } else {
      qWarning() << "error opening file " << fname << "for writing";
    }
  }

  void PlaylistsContextMenu::on_reload(const QModelIndex &index) {
    model->itemAt(index)->reload();
    model->persist();
    emit playlistChanged(model->itemAt(index));
  }
}
