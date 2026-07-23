#include "playlistcontextmenu.h"
#include "trackinfodialog.h"
#include "tageditordialog.h"
#include "reveal_in_filemanager.h"
#include "icons.h"

#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <QApplication>

namespace PlaylistUi {
  PlaylistContextMenu::PlaylistContextMenu(ProxyFilterModel *p, QTableView *v, QLineEdit *s, Config::Global &global, QObject *parent) : QObject(parent), proxy(p), view(v), search(s), global_conf(global) {
    Q_ASSERT(proxy);
    Q_ASSERT(view);
    Q_ASSERT(search);

    remove.setText(tr("Remove"));
    connect(&remove, &QAction::triggered, this, &PlaylistContextMenu::on_remove);
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

    /*QMenu move_to;
    move_to.setTitle("Move to playlist");
    move_to.addAction(&clear_filter);
    QMenu copy_to;
    copy_to.setTitle("Copy to playlist");
    copy_to.addAction(&clear_filter);
    menu.addMenu(&move_to);
    menu.addMenu(&copy_to);*/

    menu.addAction(&info);
    menu.addAction(&copy_name);
    if (proxy->modus_operandi.get() == ModusOperandi::MODUS_LOCALFS) {
      menu.addAction(&show_in_filemanager);
      bool any_editable = false;
      for (const auto &i : view->selectionModel()->selectedRows()) {
        const auto t = proxy->activeModel()->itemAt(proxy->mapToSource(i));
        if (!t.isCue() && !t.isMpd() && !t.isStream()) {
          any_editable = true;
          break;
        }
      }
      if (any_editable) {
        menu.addAction(&edit_tags);
      }
    }
    menu.addSeparator();
    menu.addAction(&remove);
    menu.exec(view->viewport()->mapToGlobal(pos));
  }

  void PlaylistContextMenu::on_remove() {
    QList <QModelIndex> lst;
    for (const auto &i : view->selectionModel()->selectedRows()) {
      lst << proxy->mapToSource(i);
    }
    proxy->activeModel()->remove(lst);
    emit playlistChanged(proxy->activeModel()->playlist());
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
}
