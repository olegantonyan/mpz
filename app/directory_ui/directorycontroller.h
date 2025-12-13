#ifndef DIRECTORYTREEVIEWMODEL_H
#define DIRECTORYTREEVIEWMODEL_H

#include "directorymodel/proxy.h"
#include "config/local.h"
#include "directorycontextmenu.h"
#include "directorysortmenu.h"
#include "modusoperandi.h"

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
    explicit Controller(QTreeView *view, QLineEdit *search, QComboBox *_libswitch, QToolButton *libcfg, QToolButton *libsort, Config::Local &local_cfg, ModusOperandi &modus, QObject *parent = nullptr);

  signals:
    void createNewPlaylist(const QList<QDir> &filepaths, const QString &libraryDir);
    void appendToCurrentPlaylist(const QList<QDir> &filepaths);

  private slots:
    void on_search(const QString& term);
    void on_doubleclick(const QModelIndex &index);
    void on_createNewPlaylist(const QList<QDir> &filepaths);

  private:
    QTreeView *view;
    DirectoryModel::Proxy *model;
    QLineEdit *search;
    Config::Local &local_conf;
    bool restore_scroll_once;
    DirectoryContextMenu *context_menu;
    DirectoryUi::SortMenu *sort_menu;
    QComboBox *libswitch;
    ModusOperandi &modus_operandi;

    void settingsDialog(QComboBox *libswitch);
    QString libraryPathMasked(const QString &libraryPath) const;

  protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
  };
}

#endif // DIRECTORYTREEVIEWMODEL_H
