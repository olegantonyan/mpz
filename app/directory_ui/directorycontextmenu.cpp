#include "directorycontextmenu.h"

#include <QMenu>
#include <QAction>
#include <QDesktopServices>
#include <QClipboard>
#include <QApplication>
#include <QUrl>

namespace DirectoryUi {
  DirectoryContextMenu::DirectoryContextMenu(Model *m, QTreeView *v, QLineEdit *s, QObject *parent) : QObject(parent), model(m), view(v), search(s) {
    Q_ASSERT(model);
    Q_ASSERT(view);
    Q_ASSERT(search);
  }

  void DirectoryContextMenu::show(const QPoint &pos) {
    auto index = view->indexAt(pos);
    if (!index.isValid()) {
      return;
    }

    auto filepath = QDir(model->filePath(index));

    QMenu menu;
    QAction clear_filter(tr("Clear filter"));
    QAction create_playlist(tr("Create new playlist"));
    QAction append_to_playlist(tr("Append to current playlist"));
    QAction open_in_filemanager(tr("Open in file manager"));

    connect(&create_playlist, &QAction::triggered, [&]() {
      emit createNewPlaylist(filepath);
    });
    connect(&append_to_playlist, &QAction::triggered, [&]() {
      emit appendToCurrentPlaylist(filepath);
    });
    connect(&open_in_filemanager, &QAction::triggered, [&]() {
      QDesktopServices::openUrl(QUrl::fromLocalFile(filepath.absolutePath()));
    });
    if (!search->text().isEmpty()) {
      connect(&clear_filter, &QAction::triggered, [&]() {
        search->clear();
      });
      menu.addAction(&clear_filter);
      menu.addSeparator();
    }

    menu.addAction(&create_playlist);
    menu.addAction(&append_to_playlist);
    menu.addAction(&open_in_filemanager);
    menu.exec(view->viewport()->mapToGlobal(pos));
  }
}
