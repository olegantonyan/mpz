#include "shortcuts.h"

#include <QGuiApplication>

Shortcuts::Shortcuts(QWidget *parent) : QObject(parent),
  _quit(parent),
  _focus_library(parent),
  _focus_playlists(parent),
  _focus_playlist(parent),
  _focus_filter_library(parent),
  _focus_filter_playlists(parent),
  _focus_filter_playlist(parent),
  _play(parent),
  _pause(parent),
  _stop(parent),
  _prev(parent),
  _next(parent),
  _open_main_menu(parent),
  _open_playback_log(parent),
  _open_sort_menu(parent),
  _open_shortcuts_menu(parent),
  _jump_to_playing_track(parent),
  _play_global(parent),
  _pause_global(parent),
  _stop_global(parent),
  _prev_global(parent),
  _next_global(parent)
{
  setupLocal();
  setupGlobal();
}

QVector<QPair<QString, QString> > Shortcuts::describe() const {
  QVector<QPair<QString, QString> > r;
  r << QPair<QString, QString>(tr("Play"), _play.key().toString());
  r << QPair<QString, QString>(tr("Stop"), _stop.key().toString());
  r << QPair<QString, QString>(tr("Pause"), _pause.key().toString());
  r << QPair<QString, QString>(tr("Next"), _next.key().toString());
  r << QPair<QString, QString>(tr("Previous"), _prev.key().toString());
  r << QPair<QString, QString>(tr("Focus on library"), _focus_library.key().toString());
  r << QPair<QString, QString>(tr("Focus on playlists"), _focus_playlists.key().toString());
  r << QPair<QString, QString>(tr("Focus on playlist"), _focus_playlist.key().toString());
  r << QPair<QString, QString>(tr("Focus on library filter"), _focus_filter_library.key().toString());
  r << QPair<QString, QString>(tr("Focus on playlists filter"), _focus_filter_playlists.key().toString());
  r << QPair<QString, QString>(tr("Focus on playlist filter"), _focus_filter_playlist.key().toString());
  r << QPair<QString, QString>(tr("Open main menu"), _open_main_menu.key().toString());
  r << QPair<QString, QString>(tr("Open playback log"), _open_playback_log.key().toString());
  r << QPair<QString, QString>(tr("Open sort menu"), _open_sort_menu.key().toString());
  r << QPair<QString, QString>(tr("Open shortcuts dialog"), _open_shortcuts_menu.key().toString());
  r << QPair<QString, QString>(tr("Jump to playing track"), _jump_to_playing_track.key().toString());
  r << QPair<QString, QString>(tr("Quit"), _quit.key().toString());
  return r;
}

void Shortcuts::setupGlobal() {
  if (QGuiApplication::platformName() == "wayland") {
    // wayland not supported and results in crash
    // https://github.com/olegantonyan/mpz/issues/129
    return;
  }
  connect(&_play_global, &QHotkey::activated, this, &Shortcuts::play);
  connect(&_pause_global, &QHotkey::activated, this, &Shortcuts::pause);
  connect(&_stop_global, &QHotkey::activated, this, &Shortcuts::stop);
  connect(&_prev_global, &QHotkey::activated, this, &Shortcuts::prev);
  connect(&_next_global, &QHotkey::activated, this, &Shortcuts::next);

  _stop_global.setShortcut(Qt::Key_MediaStop, Qt::NoModifier, true);
  _play_global.setShortcut(Qt::Key_MediaPlay, Qt::NoModifier, true);
  _pause_global.setShortcut(Qt::Key_MediaPause, Qt::NoModifier, true);
  _next_global.setShortcut(Qt::Key_MediaNext, Qt::NoModifier, true);
  _prev_global.setShortcut(Qt::Key_MediaPrevious, Qt::NoModifier, true);
}

void Shortcuts::setupLocal() {
  connect(&_quit, &QShortcut::activated, this, &Shortcuts::quit);
  connect(&_focus_library, &QShortcut::activated, this, &Shortcuts::focusLibrary);
  connect(&_focus_playlists, &QShortcut::activated, this, &Shortcuts::focusPlaylists);
  connect(&_focus_playlist, &QShortcut::activated, this, &Shortcuts::focusPlaylist);
  connect(&_focus_filter_library, &QShortcut::activated, this, &Shortcuts::focusFilterLibrary);
  connect(&_focus_filter_playlists, &QShortcut::activated, this, &Shortcuts::focusFilterPlaylists);
  connect(&_focus_filter_playlist, &QShortcut::activated, this, &Shortcuts::focusFilterPlaylist);
  connect(&_play, &QShortcut::activated, this, &Shortcuts::play);
  connect(&_pause, &QShortcut::activated, this, &Shortcuts::pause);
  connect(&_stop, &QShortcut::activated, this, &Shortcuts::stop);
  connect(&_prev, &QShortcut::activated, this, &Shortcuts::prev);
  connect(&_next, &QShortcut::activated, this, &Shortcuts::next);
  connect(&_open_main_menu, &QShortcut::activated, this, &Shortcuts::openMainMenu);
  connect(&_open_playback_log, &QShortcut::activated, this, &Shortcuts::openPlabackLog);
  connect(&_open_sort_menu, &QShortcut::activated, this, &Shortcuts::openSortMenu);
  connect(&_open_shortcuts_menu, &QShortcut::activated, this, &Shortcuts::openShortcutsMenu);
  connect(&_jump_to_playing_track, &QShortcut::activated, this, &Shortcuts::jumpToPLayingTrack);

  _quit.setKey(Qt::CTRL | Qt::Key_Q);
  _focus_library.setKey(Qt::CTRL | Qt::Key_1);
  _focus_playlists.setKey(Qt::CTRL | Qt::Key_2);
  _focus_playlist.setKey(Qt::CTRL | Qt::Key_3);
  _focus_filter_library.setKey(Qt::ALT | Qt::Key_1);
  _focus_filter_playlists.setKey(Qt::ALT | Qt::Key_2);
  _focus_filter_playlist.setKey(Qt::ALT | Qt::Key_3);
  _play.setKey(Qt::ALT | Qt::Key_E);
  _stop.setKey(Qt::ALT | Qt::Key_Q);
  _pause.setKey(Qt::ALT | Qt::Key_W);
  _prev.setKey(Qt::ALT | Qt::Key_R);
  _next.setKey(Qt::ALT | Qt::Key_T);
  _open_main_menu.setKey(Qt::ALT | Qt::Key_M);
  _open_playback_log.setKey(Qt::CTRL | Qt::Key_L);
  _open_sort_menu.setKey(Qt::CTRL | Qt::Key_S);
  _open_shortcuts_menu.setKey(Qt::ALT | Qt::Key_S);
  _jump_to_playing_track.setKey(Qt::ALT | Qt::Key_J);
}
