#ifndef SHORTCUTS_H
#define SHORTCUTS_H

#include "config/global.h"

#include <QObject>
#include <QWidget>
#include <QtGlobal>
#include <QShortcut>
#include <QDebug>

class Shortcuts : public QObject {
  Q_OBJECT
public:
  explicit Shortcuts(QWidget *parent, Config::Global &global_c);

signals:
  void quit();
  void focusLibrary();
  void focusPlaylists();
  void focusPlaylist();
  void focusFilterLibrary();
  void focusFilterPlaylists();
  void focusFilterPlaylist();

private:
  Config::Global &global_conf;

  QShortcut _quit;
  QShortcut _focus_library;
  QShortcut _focus_playlists;
  QShortcut _focus_playlist;
  QShortcut _focus_filter_library;
  QShortcut _focus_filter_playlists;
  QShortcut _focus_filter_playlist;
};

#endif // SHORTCUTS_H
