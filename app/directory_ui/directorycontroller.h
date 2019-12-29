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
#include <QLineEdit>
#include <QComboBox>

namespace DirectoryUi {
  class Controller : public QObject {
    Q_OBJECT

  public:
    explicit Controller(QTreeView *view, QLineEdit *search, QComboBox *lib_switch, Config::Local &local_cfg, QObject *parent = nullptr);

  signals:
    void createNewPlaylist(const QDir &filepath);
    void appendToCurrentPlaylist(const QDir &filepath);

  private slots:
    void on_customContextMenuRequested(const QPoint &pos);
    void on_search(const QString& term);

  private:
    QTreeView *view;
    QLineEdit *search;
    QComboBox *libswitch;
    Model *model;
    Config::Local &local_conf;
    bool restore_scroll_once;

  protected:
    bool eventFilter(QObject *obj, QEvent *event);
  };
}

#endif // DIRECTORYTREEVIEWMODEL_H
