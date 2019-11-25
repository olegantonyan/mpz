#ifndef DIRECTORYTREEVIEWMODEL_H
#define DIRECTORYTREEVIEWMODEL_H

#include "directorymodel.h"
#include "config/local.h"

#include <QObject>
#include <QTreeView>
#include <QPoint>
#include <QString>
#include <QDir>
#include <QEvent>

namespace DirectoryUi {
  class View : public QObject {
    Q_OBJECT

  public:
    explicit View(QTreeView *view, Config::Local &local_cfg, QObject *parent = nullptr);

  signals:
    void createNewPlaylist(const QDir &filepath);
    void appendToCurrentPlaylist(const QDir &filepath);

  private slots:
    void on_customContextMenuRequested(const QPoint &pos);

  private:
    QTreeView *view;
    Model *model;
    Config::Local &local_conf;

  protected:
    bool eventFilter(QObject *obj, QEvent *event);
  };
}

#endif // DIRECTORYTREEVIEWMODEL_H
