#ifndef DIRECTORYTREEVIEWMODEL_H
#define DIRECTORYTREEVIEWMODEL_H

#include <QObject>
#include <QTreeView>
#include <QPoint>
#include <QFileSystemModel>
#include <QString>

namespace Directory {
  class DirectoryViewModel : public QObject {
    Q_OBJECT

  public:
    DirectoryViewModel(QTreeView *view, const QString &library_path, QObject *parent = nullptr);

  signals:
    void createNewPlaylist(const QString &filepath);
    void appendToCurrentPlaylist(const QString &filepath);

  private slots:
    void on_customContextMenuRequested(const QPoint &pos);

  private:
    QTreeView *view;
    QFileSystemModel *model;
  };
}

#endif // DIRECTORYTREEVIEWMODEL_H
