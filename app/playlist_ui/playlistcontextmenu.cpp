#include "playlistcontextmenu.h"
#include "trackinfodialog.h"
#include "tageditordialog.h"
#include "reveal_in_filemanager.h"
#include "icons.h"
#ifdef ENABLE_DR_METER
  #include "dynamic_range_ui/dynamicrangedialog.h"
#endif

#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <QApplication>

#include <algorithm>

namespace PlaylistUi {
  PlaylistContextMenu::PlaylistContextMenu(ProxyFilterModel *p, QTableView *v, QLineEdit *s, Config::Global &global, QObject *parent) : QObject(parent), proxy(p), view(v), search(s), global_conf(global) {
    Q_ASSERT(proxy);
    Q_ASSERT(view);
    Q_ASSERT(search);

    remove.setText(tr("Remove"));
    connect(&remove, &QAction::triggered, this, &PlaylistContextMenu::removeRequested);
    remove.setIcon(Icons::get(Icons::Icon::Trash));

    show_in_filemanager.setText(tr("Show in file manager"));
    connect(&show_in_filemanager, &QAction::triggered, this, &PlaylistContextMenu::on_showInFilemanager);
    show_in_filemanager.setIcon(Icons::get(Icons::Icon::FolderReveal));

    copy_name.setText(tr("Copy name"));
    connect(&copy_name, &QAction::triggered, this, &PlaylistContextMenu::on_copyName);
    copy_name.setIcon(Icons::get(Icons::Icon::Copy));

    clear_filter.setText(tr("Clear filter"));
    connect(&clear_filter, &QAction::triggered, this, &PlaylistContextMenu::on_clearFilter);
    clear_filter.setIcon(Icons::get(Icons::Icon::Cancel));

    info.setText(tr("Track info"));
    connect(&info, &QAction::triggered, this, &PlaylistContextMenu::on_trackInfo);
    info.setIcon(Icons::get(Icons::Icon::Info));

    edit_tags.setText(tr("Edit tags…"));
    connect(&edit_tags, &QAction::triggered, this, &PlaylistContextMenu::on_editTags);
    edit_tags.setIcon(Icons::get(Icons::Icon::Details));

#ifdef ENABLE_DR_METER
    dynamic_range.setText(tr("Dynamic range…"));
    connect(&dynamic_range, &QAction::triggered, this, &PlaylistContextMenu::on_dynamicRange);
    dynamic_range.setIcon(Icons::get(Icons::Icon::DynamicRange));
#endif
  }

  QVector<Track> PlaylistContextMenu::selectedTracks() const {
    QModelIndexList rows = view->selectionModel()->selectedRows();
    std::sort(rows.begin(), rows.end(),
              [](const QModelIndex &a, const QModelIndex &b) { return a.row() < b.row(); });

    QVector<Track> tracks;
    tracks.reserve(rows.size());
    for (const auto &i : std::as_const(rows)) {
      const auto t = proxy->activeModel()->itemAt(proxy->mapToSource(i));
      if (!t.isMpd() && !t.isStream()) {
        tracks << t;
      }
    }
    return tracks;
  }

  void PlaylistContextMenu::show(const QPoint &pos) {
    if(!view->indexAt(pos).isValid()) {
      return;
    }

    QMenu menu;
    if (!search->text().isEmpty()) {
       menu.addAction(&clear_filter);
       menu.addSeparator();
    }

    menu.addAction(&info);
    menu.addAction(&copy_name);
    if (proxy->modus_operandi.get() == ModusOperandi::MODUS_LOCALFS) {
      menu.addAction(&show_in_filemanager);
      bool any_editable = false;
      bool any_local = false;
      for (const auto &i : view->selectionModel()->selectedRows()) {
        const auto t = proxy->activeModel()->itemAt(proxy->mapToSource(i));
        if (t.isMpd() || t.isStream()) {
          continue;
        }
        any_local = true;
        if (!t.isCue()) {
          any_editable = true;
        }
      }
      if (any_editable) {
        menu.addAction(&edit_tags);
      }
#ifdef ENABLE_DR_METER
      if (any_local) {
        menu.addAction(&dynamic_range);
      }
#endif
    }
    menu.addSeparator();
    menu.addAction(&remove);
    menu.exec(view->viewport()->mapToGlobal(pos));
  }

  void PlaylistContextMenu::on_clearFilter() {
    search->clear();
  }

  void PlaylistContextMenu::on_copyName() {
    QStringList str;
    for (const auto &i : view->selectionModel()->selectedRows()) {
      str << proxy->activeModel()->itemAt(proxy->mapToSource(i)).formattedTitle();
    }
    qApp->clipboard()->setText(str.join('\n'));
  }

  void PlaylistContextMenu::on_showInFilemanager() {
    QStringList paths;
    for (const auto &i : view->selectionModel()->selectedRows()) {
      paths << proxy->activeModel()->itemAt(proxy->mapToSource(i)).path();
    }
    revealInFileManager(paths);
  }

  void PlaylistContextMenu::on_trackInfo() {
    auto rows = view->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
      return;
    }
    auto selection = rows.first();
    if (selection.isValid()) {
      auto track = proxy->activeModel()->itemAt(proxy->mapToSource(selection));
      auto pl = proxy->activeModel()->playlist();
      TrackInfoDialog *dlg = new TrackInfoDialog(track, global_conf, pl);
      dlg->setModal(false);
      connect(dlg, &TrackInfoDialog::finished, dlg, &TrackInfoDialog::deleteLater);
      connect(dlg, &TrackInfoDialog::tagEditorOpened, this, [this, pl](TagEditorDialog *editor) {
        connect(editor, &TagEditorDialog::saved, this, [this, pl](const QList<quint64> &uids) {
          emit tracksChanged(pl, uids);
        });
      });
      dlg->show();
    }
  }

  void PlaylistContextMenu::on_editTags() {
    QVector<Track> editable;
    for (const auto &i : view->selectionModel()->selectedRows()) {
      const auto t = proxy->activeModel()->itemAt(proxy->mapToSource(i));
      if (!t.isCue() && !t.isMpd() && !t.isStream()) {
        editable << t;
      }
    }
    if (editable.isEmpty()) {
      return;
    }
    auto pl = proxy->activeModel()->playlist();
    TagEditorDialog *dlg = new TagEditorDialog(editable, pl);
    dlg->setModal(false);
    connect(dlg, &TagEditorDialog::finished, dlg, &TagEditorDialog::deleteLater);
    connect(dlg, &TagEditorDialog::saved, this, [this, pl](const QList<quint64> &uids) {
      emit tracksChanged(pl, uids);
    });
    dlg->show();
  }

#ifdef ENABLE_DR_METER
  void PlaylistContextMenu::on_dynamicRange() {
    const QVector<Track> tracks = selectedTracks();
    if (tracks.isEmpty()) {
      return;
    }
    DynamicRangeDialog *dlg = new DynamicRangeDialog(tracks);
    dlg->setModal(false);
    connect(dlg, &DynamicRangeDialog::finished, dlg, &DynamicRangeDialog::deleteLater);
    dlg->show();
  }
#endif
}
