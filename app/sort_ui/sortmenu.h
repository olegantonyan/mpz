#ifndef SORTMENU_H
#define SORTMENU_H

#include "config/global.h"

#include <QObject>
#include <QPushButton>

namespace SortUi {
  class SortMenu : public QObject {
    Q_OBJECT
  public:
    explicit SortMenu(QPushButton *button, Config::Global &global_c);

  signals:
    void triggered(const QString& criteria);

  private:
    Config::Global &global_conf;
  };
}

#endif // SORTMENU_H
