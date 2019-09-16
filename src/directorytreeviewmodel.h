#ifndef DIRECTORYTREEVIEWMODEL_H
#define DIRECTORYTREEVIEWMODEL_H

#include <QObject>
#include <QTreeView>
#include <QMenu>
#include <QPoint>
#include <QFileSystemModel>
#include <QString>

class DirectoryTreeViewModel : public QObject {
  Q_OBJECT

public:
  DirectoryTreeViewModel(QTreeView *view, const QString &library_path, QObject *parent = nullptr);

private slots:
  void on_customContextMenuRequested(const QPoint &pos);

private:
  QTreeView *view;
  QFileSystemModel *model;
  QMenu *context_menu;
};

#endif // DIRECTORYTREEVIEWMODEL_H
