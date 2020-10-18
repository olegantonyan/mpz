#ifndef SORTMENU_H
#define SORTMENU_H

#include "config/global.h"

#include <QObject>
#include <QToolButton>
#include <QAction>

namespace SortUi {
  class SortMenu : public QObject {
    Q_OBJECT
  public:
    explicit SortMenu(QToolButton *button, Config::Global &global_c);

  signals:
    void triggered(const QString& criteria);

  private slots:
    void on_open();
    void on_action_triggered(QAction *action);

  private:
    QToolButton *button;
    Config::Global &global_conf;

    void showEditPresetsDialog();
  };
}

#endif // SORTMENU_H
