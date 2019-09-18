#ifndef DIRECTORYTREEVIEWMODEL_H
#define DIRECTORYTREEVIEWMODEL_H

#include "directorydatamodel.h"

#include <QObject>
#include <QTreeView>
#include <QPoint>
#include <QFileSystemModel>
#include <QString>
#include <QDir>

namespace Directory {
  class DirectoryViewModel : public QObject {
    Q_OBJECT

  public:
    explicit DirectoryViewModel(QTreeView *view, const QString &library_path, QObject *parent = nullptr);

  signals:
    void createNewPlaylist(const QDir &filepath);
    void appendToCurrentPlaylist(const QDir &filepath);

  private slots:
    void on_customContextMenuRequested(const QPoint &pos);

  private:
    QTreeView *view;
    Directory::DirectoryDataModel *model;
  };
}

#endif // DIRECTORYTREEVIEWMODEL_H
