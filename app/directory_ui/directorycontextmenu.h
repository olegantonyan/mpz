#ifndef DIRECTORYCONTEXTMENU_H
#define DIRECTORYCONTEXTMENU_H

#include "directorymodel.h"
#include "playlist.h"

#include <QObject>
#include <QPoint>
#include <QTreeView>
#include <QLineEdit>
#include <memory>
#include <QAction>
#include <QDir>

namespace DirectoryUi {
  class DirectoryContextMenu : public QObject {
    Q_OBJECT
  public:
    explicit DirectoryContextMenu(Model *model, QTreeView *view, QLineEdit *seacrh, QObject *parent = nullptr);

  public slots:
    void show(const QPoint &pos);

  signals:
    void createNewPlaylist(const QDir &filepath);
    void appendToCurrentPlaylist(const QDir &filepath);

  private:
    Model *model;
    QTreeView *view;
    QLineEdit *search;
  };
}

#endif // DIRECTORYCONTEXTMENU_H
