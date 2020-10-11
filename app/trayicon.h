#ifndef TRAYICON_H
#define TRAYICON_H

#include "track.h"
#include "config/global.h"

#include <QObject>
#include <QIcon>
#include <QSystemTrayIcon>
#include <QMainWindow>
#include <QMenu>

class TrayIcon : public QObject {
  Q_OBJECT
public:
  explicit TrayIcon(QMainWindow *parent, Config::Global &global_c);

signals:
  void startTriggered();
  void stopTriggered();
  void nextTriggered();
  void prevTriggered();
  void pauseTriggered();

  void clicked();

public slots:
  void hide();
  void on_playerStarted(const Track &track);
  void on_playerStopped();
  void on_playerPaused(const Track &track);
  void on_playerProgress(const Track &track, int current_seconds);

private:
  QSystemTrayIcon *trayicon;
  QMenu *menu;

  Config::Global &global_conf;

  QAction *quit;
  QAction *play;
  QAction *pause;
  QAction *stop;
  QAction *next;
  QAction *prev;
  QAction *now_playing;
  QString time_text(const Track &track, int pos) const;
  void update_menu_now_playing(const Track &track, int pos);
};

#endif // TRAYICON_H
