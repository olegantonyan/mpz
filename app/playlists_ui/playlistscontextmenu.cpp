#include "playlistscontextmenu.h"
#include "icons.h"

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
  PlaylistsContextMenu::PlaylistsContextMenu(ProxyFilterModel *m, QListView *v, QLineEdit *s, QObject *parent) : QObject(parent), model(m), view(v), search(s) {
    Q_ASSERT(model);
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
    QAction loadm3u(tr("Load m3u"));
    QAction reload(tr("Reload from filesystem"));
    QAction play(tr("Play"));

    connect(&reload, &QAction::triggered, this, [&]() {
      on_reload(index);
    });
    reload.setIcon(Icons::get(Icons::Icon::Reload));

    remove.setIcon(Icons::get(Icons::Icon::Trash));
    savem3u.setIcon(Icons::get(Icons::Icon::Save));
    play.setIcon(Icons::get(Icons::Icon::Play));

    connect(&remove, &QAction::triggered, this, [&]() {
      emit removed(index);
    });

    connect(&rename, &QAction::triggered, this, [&]() {
      on_rename(index);
    });

    QAction clear_filter(tr("Clear filter"));
    clear_filter.setIcon(Icons::get(Icons::Icon::Cancel));
    if (!search->text().isEmpty()) {
       connect(&clear_filter, &QAction::triggered, this, [=]() {
         search->clear();
       });
       menu.addAction(&clear_filter);
       menu.addSeparator();
    }

    connect(&savem3u, &QAction::triggered, this, [&]() {
      on_savem3u(index);
    });

    connect(&play, &QAction::triggered, this, [&]() {
      emit view->doubleClicked(index);
    });

    connect(&loadm3u, &QAction::triggered, this, [&]() {
      QStringList files = QFileDialog::getOpenFileNames(view, tr("Select playlist files"), QStandardPaths::writableLocation(QStandardPaths::MusicLocation), "Playlists (*.m3u *.pls)");
      emit loadPlaylistFiles(index, files);
    });

    menu.addAction(&play);
    menu.addSeparator();
    menu.addAction(&rename);
    menu.addAction(&savem3u);
    menu.addAction(&loadm3u);
    menu.addAction(&reload);
    menu.addSeparator();
    menu.addAction(&remove);
    menu.exec(view->viewport()->mapToGlobal(pos));
  }

  void PlaylistsContextMenu::on_rename(const QModelIndex &index)  {
    auto i = model->itemAt(index);
    bool ok;
    QString new_name = QInputDialog::getText(view, QString("%1 '%2'").arg(tr("Rename playlist")).arg(i->name()), "", QLineEdit::Normal, i->name(), &ok, Qt::Widget);
    if (ok && !new_name.isEmpty()) {
      QString old_name = i->name();
      i->rename(new_name);
      model->persist();
      emit renamed(old_name, new_name);
    }
  }

  void PlaylistsContextMenu::on_savem3u(const QModelIndex &index) {
    auto i = model->itemAt(index);
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
