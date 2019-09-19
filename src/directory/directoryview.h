#ifndef DIRECTORYTREEVIEWMODEL_H
#define DIRECTORYTREEVIEWMODEL_H

#include "directorymodel.h"

#include <QObject>
#include <QTreeView>
#include <QPoint>
#include <QFileSystemModel>
#include <QString>
#include <QDir>

namespace Directory {
  class View : public QObject {
    Q_OBJECT

  public:
    explicit View(QTreeView *view, const QString &library_path, QObject *parent = nullptr);

  signals:
    void createNewPlaylist(const QDir &filepath);
    void appendToCurrentPlaylist(const QDir &filepath);

  private slots:
    void on_customContextMenuRequested(const QPoint &pos);

  private:
    QTreeView *view;
    Directory::Model *model;
  };
}

#endif // DIRECTORYTREEVIEWMODEL_H
