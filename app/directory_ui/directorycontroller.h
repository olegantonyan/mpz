#ifndef DIRECTORYTREEVIEWMODEL_H
#define DIRECTORYTREEVIEWMODEL_H

#include "directorymodel.h"
#include "config/local.h"
#include "directorycontextmenu.h"

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
    explicit Controller(QTreeView *view, QLineEdit *search, QComboBox *libswitch, QToolButton *libcfg, Config::Local &local_cfg, QObject *parent = nullptr);

  signals:
    void createNewPlaylist(const QDir &filepath);
    void appendToCurrentPlaylist(const QDir &filepath);

  private slots:
    void on_search(const QString& term);
    void on_doubleclick(const QModelIndex &index);

  private:
    QTreeView *view;
    Model *model;
    QLineEdit *search;
    Config::Local &local_conf;
    bool restore_scroll_once;
    DirectoryContextMenu *context_menu;

    void settingsDialog(QComboBox *libswitch);
    
  protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
  };
}

#endif // DIRECTORYTREEVIEWMODEL_H
