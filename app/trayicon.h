#ifndef TRAYICON_H
#define TRAYICON_H

#include "playback/playbackview.h"

#include <QObject>
#include <QIcon>
#include <QSystemTrayIcon>
#include <QMainWindow>
#include <QMenu>

class TrayIcon : public QObject {
  Q_OBJECT
public:
  explicit TrayIcon(Playback::View *player, const QIcon &appicon, QMainWindow *parent = nullptr);

signals:

public slots:
  void hide();

private:
  Playback::View *player;
  QSystemTrayIcon *trayicon;
  QMenu *menu;
};

#endif // TRAYICON_H
