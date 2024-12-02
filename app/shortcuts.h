#ifndef SHORTCUTS_H
#define SHORTCUTS_H

#include "config/global.h"
#include "QHotkey/qhotkey.h"

#include <QObject>
#include <QWidget>
#include <QtGlobal>
#include <QShortcut>
#include <QDebug>
#include <QVector>
#include <QPair>

class Shortcuts : public QObject {
  Q_OBJECT
public:
  explicit Shortcuts(QWidget *parent);

  QVector<QPair<QString, QString>> describe() const;

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
  void openSortMenu();
  void openShortcutsMenu();
  void jumpToPLayingTrack();

private:
  void setupGlobal();
  void setupLocal();

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
  QShortcut _open_sort_menu;
  QShortcut _open_shortcuts_menu;
  QShortcut _jump_to_playing_track;

  QHotkey _play_global;
  QHotkey _pause_global;
  QHotkey _stop_global;
  QHotkey _prev_global;
  QHotkey _next_global;
};

#endif // SHORTCUTS_H
