#ifndef TRAYICON_H
#define TRAYICON_H

#include "playback/playbackcontroller.h"

#include <QObject>
#include <QIcon>
#include <QSystemTrayIcon>
#include <QMainWindow>
#include <QMenu>

class TrayIcon : public QObject {
  Q_OBJECT
public:
  explicit TrayIcon(Playback::Controller *player, const QIcon &appicon, QMainWindow *parent = nullptr);

signals:

public slots:
  void hide();

private:
  Playback::Controller *player;
  QSystemTrayIcon *trayicon;
  QMenu *menu;
};

#endif // TRAYICON_H
