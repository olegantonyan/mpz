#ifndef MAINMENU_H
#define MAINMENU_H

#include "config/global.h"
#include "config/local.h"
#include "modusoperandi.h"

#include <QObject>
#include <QToolButton>
#include <QList>

class QAction;

class MainMenu : public QObject {
  Q_OBJECT
public:
  explicit MainMenu(QToolButton *btn, Config::Global &global_c, Config::Local &local_c, ModusOperandi &modus);

  void setViewActions(const QList<QAction *> &actions);

signals:
  void exit();
  void toggleTrayIcon();
  void openPlaybackLog();
  void openShortcuts();
  void openEqualizer();

public slots:
  void on_open();

private:
  QToolButton *button;
  QList<QAction *> view_actions;

  Config::Global &global_conf;
  Config::Local &local_conf;
  ModusOperandi &modus_operandi;
};

#endif // MAINMENU_H
