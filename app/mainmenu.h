#ifndef MAINMENU_H
#define MAINMENU_H

#include "config/global.h"
#include "config/local.h"

#include <QObject>
#include <QToolButton>

class MainMenu : public QObject {
  Q_OBJECT
public:
  explicit MainMenu(QToolButton *btn, Config::Global &global_c, Config::Local &local_c);

signals:
  void exit();
  void toggleTrayIcon();
  void openPlaybackLog();
  void openShortcuts();

public slots:
  void on_open();

private:
  QToolButton *button;

  Config::Global &global_conf;
  Config::Local &local_conf;
};

#endif // MAINMENU_H
