#ifndef TRAYICON_H
#define TRAYICON_H

#include "track.h"

#include <QObject>
#include <QIcon>
#include <QSystemTrayIcon>
#include <QMainWindow>
#include <QMenu>

class TrayIcon : public QObject {
  Q_OBJECT
public:
  explicit TrayIcon(const QIcon &appicon, QMainWindow *parent = nullptr);

signals:
  void startTriggered();
  void stopTriggered();
  void nextTriggered();
  void prevTriggered();
  void pauseTriggered();

public slots:
  void hide();
  void on_playerStarted(const Track &track);
  void on_playerStopped();
  void on_playerPaused(const Track &track);

private:
  QSystemTrayIcon *trayicon;
  QMenu *menu;

  QAction *quit;
  QAction *play;
  QAction *pause;
  QAction *stop;
  QAction *next;
  QAction *prev;
  QAction *now_playing;
};

#endif // TRAYICON_H
