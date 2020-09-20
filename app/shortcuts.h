#ifndef SHORTCUTS_H
#define SHORTCUTS_H

#include "config/global.h"
#include "QHotkey/qhotkey.h"

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
  void play();
  void pause();
  void stop();
  void prev();
  void next();
  void openMainMenu();
  void openPlabackLog();

private:
  void setupGlobal();
  void setupLocal();

  Config::Global &global_conf;

  QShortcut _quit;
  QShortcut _focus_library;
  QShortcut _focus_playlists;
  QShortcut _focus_playlist;
  QShortcut _focus_filter_library;
  QShortcut _focus_filter_playlists;
  QShortcut _focus_filter_playlist;
  QShortcut _play;
  QShortcut _pause;
  QShortcut _stop;
  QShortcut _prev;
  QShortcut _next;
  QShortcut _open_main_menu;
  QShortcut _open_playback_log;

  QHotkey _play_global;
  QHotkey _pause_global;
  QHotkey _stop_global;
  QHotkey _prev_global;
  QHotkey _next_global;
};

#endif // SHORTCUTS_H
