#ifndef MAINMENU_H
#define MAINMENU_H

#include "config/global.h"

#include <QObject>
#include <QToolButton>

class MainMenu : public QObject {
  Q_OBJECT
public:
  explicit MainMenu(QToolButton *btn, Config::Global &global_c);

signals:
  void exit();
  void toggleTrayIcon();
  void openPlaybackLog();

public slots:
  void on_open();

private:
  QToolButton *button;

  Config::Global &global_conf;
};

#endif // MAINMENU_H
