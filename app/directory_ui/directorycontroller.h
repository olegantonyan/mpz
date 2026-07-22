#ifndef DIRECTORYTREEVIEWMODEL_H
#define DIRECTORYTREEVIEWMODEL_H

#include "directorymodel/proxy.h"
#include "config/local.h"
#include "config/global.h"
#include "directorycontextmenu.h"
#include "directorysortmenu.h"
#include "modusoperandi.h"
#include "track.h"

#include <QObject>
#include <QTreeView>
#include <QPoint>
#include <QString>
#include <QDir>
#include <QEvent>
#include <QLineEdit>
#include <QComboBox>
#include <QToolButton>

namespace DirectoryUi {
  class Controller : public QObject {
    Q_OBJECT

  public:
    explicit Controller(QTreeView *view, QLineEdit *search, QComboBox *_libswitch, QToolButton *libcfg, QToolButton *libsort, Config::Local &local_cfg, Config::Global &global_cfg, ModusOperandi &modus, QObject *parent = nullptr);

  signals:
    void createNewPlaylist(const QList<QDir> &filepaths, const QString &libraryDir);
    void appendToCurrentPlaylist(const QList<QDir> &filepaths);
    // Radio bypasses the QDir payload: QDir mangles a stream URL (it drops the
    // trailing slash) and Playlist::Loader only scans the filesystem.
    void createNewPlaylistFromTracks(const QVector<Track> &tracks, const QString &name);
    void appendTracksToCurrentPlaylist(const QVector<Track> &tracks);

  private slots:
    void on_search(const QString& term);
    void on_doubleclick(const QModelIndex &index);
    void on_createNewPlaylist(const QList<QDir> &filepaths);

  private:
    QTreeView *view;
    DirectoryModel::Proxy *model;
    QLineEdit *search;
    Config::Local &local_conf;
    Config::Global &global_conf;
    bool restore_scroll_once;
    DirectoryContextMenu *context_menu;
    DirectoryUi::SortMenu *sort_menu;
    QComboBox *libswitch;
    QToolButton *libsort;
    ModusOperandi &modus_operandi;

    void settingsDialog(QComboBox *libswitch);
    void populateLibrarySwitch();
    void selectLibrary(const QString &path);
    void editStations();
    bool radioMode() const;
    // Returns true when the index was handled as a radio station/group.
    bool emitRadioTracks(const QModelIndexList &indexes, bool append);

  protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
  };
}

#endif // DIRECTORYTREEVIEWMODEL_H
