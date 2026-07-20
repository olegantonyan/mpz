#include "directorycontextmenu.h"
#include "icons.h"

#include <QMenu>
#include <QAction>
#include <QDesktopServices>
#include <QClipboard>
#include <QApplication>
#include <QUrl>

namespace DirectoryUi {
  DirectoryContextMenu::DirectoryContextMenu(DirectoryModel::Proxy *m, QTreeView *v, QLineEdit *s, QObject *parent) : QObject(parent), model(m), view(v), search(s) {
    Q_ASSERT(model);
    Q_ASSERT(view);
    Q_ASSERT(search);
  }

  void DirectoryContextMenu::showRadioMenu(const QPoint &pos, const QModelIndex &index) {
    QModelIndexList selected = view->selectionModel()->selectedRows();
    if (!selected.contains(index)) {
      selected = QModelIndexList{index};
    }
    const auto tracks = model->tracksAt(selected);
    const auto homepage = index.data(DirectoryModel::RadioRole::Homepage).toString();

    QMenu menu;
    QAction clear_filter(tr("Clear filter"));
    clear_filter.setIcon(Icons::get(Icons::Icon::Cancel));
    QAction create_playlist(tr("Create new playlist"));
    create_playlist.setIcon(Icons::get(Icons::Icon::NewPlaylist));
    QAction append_to_playlist(tr("Append to current playlist"));
    append_to_playlist.setIcon(Icons::get(Icons::Icon::AddToPlaylist));
    QAction open_homepage(tr("Open station homepage"));
    open_homepage.setIcon(Icons::get(Icons::Icon::Info));
    QAction edit_stations(tr("Edit stations..."));
    edit_stations.setIcon(Icons::get(Icons::Icon::Edit));
    QAction reload_stations(tr("Reload stations"));
    reload_stations.setIcon(Icons::get(Icons::Icon::Reload));
    QAction reset_stations(tr("Reset to built-in stations"));
    reset_stations.setIcon(Icons::get(Icons::Icon::Trash));

    connect(&create_playlist, &QAction::triggered, this, [&]() {
      emit createNewPlaylistFromTracks(tracks, model->displayName(index));
    });
    connect(&append_to_playlist, &QAction::triggered, this, [&]() {
      emit appendTracksToCurrentPlaylist(tracks);
    });
    connect(&open_homepage, &QAction::triggered, this, [&]() {
      QDesktopServices::openUrl(QUrl(homepage));
    });
    connect(&edit_stations, &QAction::triggered, this, [&]() { emit editStations(); });
    connect(&reload_stations, &QAction::triggered, this, [&]() { emit reloadStations(); });
    connect(&reset_stations, &QAction::triggered, this, [&]() { emit resetStations(); });

    if (!search->text().isEmpty()) {
      connect(&clear_filter, &QAction::triggered, this, [&]() { search->clear(); });
      menu.addAction(&clear_filter);
      menu.addSeparator();
    }

    create_playlist.setEnabled(!tracks.isEmpty());
    append_to_playlist.setEnabled(!tracks.isEmpty());
    menu.addAction(&create_playlist);
    menu.addAction(&append_to_playlist);
    if (!homepage.isEmpty()) {
      menu.addAction(&open_homepage);
    }
    menu.addSeparator();
    menu.addAction(&edit_stations);
    menu.addAction(&reload_stations);
    menu.addAction(&reset_stations);
    menu.exec(view->viewport()->mapToGlobal(pos));
  }

  void DirectoryContextMenu::show(const QPoint &pos) {
    auto index = view->indexAt(pos);
    if (!index.isValid()) {
      return;
    }
    if (model->isRadioActive()) {
      showRadioMenu(pos, index);
      return;
    }
    QList<QDir> selected_dirs;
    for (const auto &i : view->selectionModel()->selectedRows()) {
      selected_dirs << QDir(model->filePath(i));
    }

    auto filepath = QDir(model->filePath(index));

    QMenu menu;
    QAction clear_filter(tr("Clear filter"));
    clear_filter.setIcon(Icons::get(Icons::Icon::Cancel));
    QAction create_playlist(tr("Create new playlist"));
    create_playlist.setIcon(Icons::get(Icons::Icon::NewPlaylist));

    QAction append_to_playlist(tr("Append to current playlist"));
    append_to_playlist.setIcon(Icons::get(Icons::Icon::AddToPlaylist));
    QAction open_in_filemanager(tr("Open in file manager"));
    open_in_filemanager.setIcon(Icons::get(Icons::Icon::FolderReveal));

    connect(&create_playlist, &QAction::triggered, this, [&]() {
      emit createNewPlaylist(selected_dirs);
    });
    connect(&append_to_playlist, &QAction::triggered, this, [&]() {
      emit appendToCurrentPlaylist(selected_dirs);
    });
    connect(&open_in_filemanager, &QAction::triggered, [&]() {
      QDesktopServices::openUrl(QUrl::fromLocalFile(filepath.absolutePath()));
    });
    if (!search->text().isEmpty()) {
      connect(&clear_filter, &QAction::triggered, this, [&]() {
        search->clear();
      });
      menu.addAction(&clear_filter);
      menu.addSeparator();
    }

    menu.addAction(&create_playlist);
    menu.addAction(&append_to_playlist);
    if (model->modus_operandi.get() == ModusOperandi::MODUS_LOCALFS) {
      menu.addAction(&open_in_filemanager);
    }
    menu.exec(view->viewport()->mapToGlobal(pos));
  }
}
