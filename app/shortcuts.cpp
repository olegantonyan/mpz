#include "shortcuts.h"

Shortcuts::Shortcuts(QWidget *parent, Config::Global &global_c) : QObject(parent), global_conf(global_c),
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
  _next(parent)
{
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
}
