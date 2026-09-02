#include "shortcuts.h"

#include <QGuiApplication>
#include <QDebug>

Shortcuts::Shortcuts(Config::Global &global_c, Config::Local &local_c, QWidget *parent) : QObject(parent)
  , _parent(parent)
  , global_conf(global_c)
  , local_conf(local_c)
  , _specs(resolve(global_c.shortcuts()))
#ifdef ENABLE_QHOTKEY
#ifdef Q_OS_WIN
  , _playpause_global(parent)
#else
  , _play_global(parent)
  , _pause_global(parent)
#endif
  , _stop_global(parent)
  , _prev_global(parent)
  , _next_global(parent)
#endif
{
  setupLocal();
  setupGlobal();
}

const QVector<Shortcuts::Spec> &Shortcuts::defaults() {
  // keys must stay identical across platforms, global.yml is portable
  static const QVector<Spec> table = []() {
    QVector<Spec> t;
#ifdef Q_OS_MACOS
    t << Spec{Action::PlayPause, "play_pause", tr("Play / Pause"), QKeySequence(Qt::Key_Space), false};
    t << Spec{Action::Play, "play", QString(), QKeySequence(), false};
    t << Spec{Action::Pause, "pause", QString(), QKeySequence(), false};
    t << Spec{Action::Stop, "stop", QString(), QKeySequence(), false};
    t << Spec{Action::Next, "next", tr("Next"), QKeySequence(Qt::CTRL | Qt::Key_Right), false};
    t << Spec{Action::Prev, "prev", tr("Previous"), QKeySequence(Qt::CTRL | Qt::Key_Left), false};
    t << Spec{Action::VolumeUp, "volume_up", tr("Volume up"), QKeySequence(Qt::CTRL | Qt::Key_Up), false};
    t << Spec{Action::VolumeDown, "volume_down", tr("Volume down"), QKeySequence(Qt::CTRL | Qt::Key_Down), false};
    t << Spec{Action::Settings, "settings", tr("Settings"), QKeySequence(QKeySequence::Preferences), false};
    t << Spec{Action::FocusLibrary, "focus_library", tr("Focus on library"), QKeySequence(Qt::CTRL | Qt::Key_1), true};
    t << Spec{Action::FocusPlaylists, "focus_playlists", tr("Focus on playlists"), QKeySequence(Qt::CTRL | Qt::Key_2), true};
    t << Spec{Action::FocusPlaylist, "focus_playlist", tr("Focus on playlist"), QKeySequence(Qt::CTRL | Qt::Key_3), true};
    // Cmd+Option+digit, not Cmd+Shift+digit: the latter clashes with macOS system screenshot shortcuts (⌘⇧3/⌘⇧4/⌘⇧5).
    t << Spec{Action::FocusFilterLibrary, "focus_filter_library", tr("Focus on library filter"), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_1), true};
    t << Spec{Action::FocusFilterPlaylists, "focus_filter_playlists", tr("Focus on playlists filter"), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_2), true};
    t << Spec{Action::FocusFilterPlaylist, "focus_filter_playlist", tr("Focus on playlist filter"), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_3), true};
    t << Spec{Action::OpenMainMenu, "open_main_menu", tr("Open main menu"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_M), true};
    t << Spec{Action::OpenPlaybackLog, "open_playback_log", tr("Open playback log"), QKeySequence(Qt::CTRL | Qt::Key_L), true};
    t << Spec{Action::OpenSortMenu, "open_sort_menu", tr("Open sort menu"), QKeySequence(Qt::CTRL | Qt::Key_S), true};
#ifdef ENABLE_DEVICES_MENU
    t << Spec{Action::OpenOutputMenu, "open_output_menu", tr("Open output device menu"), QKeySequence(Qt::CTRL | Qt::Key_D), true};
#endif
    t << Spec{Action::OpenShortcutsMenu, "open_shortcuts_dialog", tr("Open shortcuts dialog"), QKeySequence(Qt::CTRL | Qt::Key_Slash), true};
    t << Spec{Action::JumpToPlayingTrack, "jump_to_playing_track", tr("Jump to playing track"), QKeySequence(Qt::CTRL | Qt::Key_J), false};
    t << Spec{Action::Quit, "quit", tr("Quit"), QKeySequence(QKeySequence::Quit), false};
#else
    t << Spec{Action::PlayPause, "play_pause", tr("Play / Pause"), QKeySequence(Qt::Key_Space), true};
    t << Spec{Action::Play, "play", tr("Play"), QKeySequence(Qt::ALT | Qt::Key_E), true};
    t << Spec{Action::Pause, "pause", tr("Pause"), QKeySequence(Qt::ALT | Qt::Key_W), true};
    t << Spec{Action::Stop, "stop", tr("Stop"), QKeySequence(Qt::ALT | Qt::Key_Q), true};
    t << Spec{Action::Next, "next", tr("Next"), QKeySequence(Qt::ALT | Qt::Key_T), true};
    t << Spec{Action::Prev, "prev", tr("Previous"), QKeySequence(Qt::ALT | Qt::Key_R), true};
    t << Spec{Action::VolumeUp, "volume_up", tr("Volume up"), QKeySequence(Qt::CTRL | Qt::Key_Up), true};
    t << Spec{Action::VolumeDown, "volume_down", tr("Volume down"), QKeySequence(Qt::CTRL | Qt::Key_Down), true};
    t << Spec{Action::Settings, "settings", tr("Settings"), QKeySequence(Qt::CTRL | Qt::Key_Comma), true};
    t << Spec{Action::FocusLibrary, "focus_library", tr("Focus on library"), QKeySequence(Qt::CTRL | Qt::Key_1), true};
    t << Spec{Action::FocusPlaylists, "focus_playlists", tr("Focus on playlists"), QKeySequence(Qt::CTRL | Qt::Key_2), true};
    t << Spec{Action::FocusPlaylist, "focus_playlist", tr("Focus on playlist"), QKeySequence(Qt::CTRL | Qt::Key_3), true};
    t << Spec{Action::FocusFilterLibrary, "focus_filter_library", tr("Focus on library filter"), QKeySequence(Qt::ALT | Qt::Key_1), true};
    t << Spec{Action::FocusFilterPlaylists, "focus_filter_playlists", tr("Focus on playlists filter"), QKeySequence(Qt::ALT | Qt::Key_2), true};
    t << Spec{Action::FocusFilterPlaylist, "focus_filter_playlist", tr("Focus on playlist filter"), QKeySequence(Qt::ALT | Qt::Key_3), true};
    t << Spec{Action::OpenMainMenu, "open_main_menu", tr("Open main menu"), QKeySequence(Qt::ALT | Qt::Key_M), true};
    t << Spec{Action::OpenPlaybackLog, "open_playback_log", tr("Open playback log"), QKeySequence(Qt::CTRL | Qt::Key_L), true};
    t << Spec{Action::OpenSortMenu, "open_sort_menu", tr("Open sort menu"), QKeySequence(Qt::CTRL | Qt::Key_S), true};
#ifdef ENABLE_DEVICES_MENU
    t << Spec{Action::OpenOutputMenu, "open_output_menu", tr("Open output device menu"), QKeySequence(Qt::CTRL | Qt::Key_D), true};
#endif
    t << Spec{Action::OpenShortcutsMenu, "open_shortcuts_dialog", tr("Open shortcuts dialog"), QKeySequence(Qt::ALT | Qt::Key_S), true};
    t << Spec{Action::JumpToPlayingTrack, "jump_to_playing_track", tr("Jump to playing track"), QKeySequence(Qt::ALT | Qt::Key_J), true};
    t << Spec{Action::Quit, "quit", tr("Quit"), QKeySequence(Qt::CTRL | Qt::Key_Q), true};
#endif
    return t;
  }();
  return table;
}

QVector<Shortcuts::Spec> Shortcuts::resolve(const QMap<QString, QString> &overrides) {
  QVector<Spec> table = defaults();

  QVector<bool> overridden(table.size(), false);
  for (int i = 0; i < table.size(); i++) {
    auto it = overrides.constFind(table[i].key);
    if (it == overrides.constEnd()) {
      continue;
    }
    if (it->isEmpty()) {
      table[i].sequence = QKeySequence();
      overridden[i] = true;
      continue;
    }
    // garbage parses to Key_unknown, which is not isEmpty() but stringifies to nothing
    auto seq = QKeySequence::fromString(*it, QKeySequence::PortableText);
    if (seq.isEmpty() || seq.toString(QKeySequence::PortableText).isEmpty()) {
      qWarning() << "ignoring unparseable shortcut for" << table[i].key << *it;
      continue;
    }
    table[i].sequence = seq;
    overridden[i] = true;
  }

  // shared sequences fire activatedAmbiguously() and nothing else, killing both keys
  for (int i = 0; i < table.size(); i++) {
    if (table[i].sequence.isEmpty()) {
      continue;
    }
    for (int j = i + 1; j < table.size(); j++) {
      if (table[j].sequence != table[i].sequence) {
        continue;
      }
      int loser = (overridden[j] && !overridden[i]) ? i : j;
      int winner = loser == i ? j : i;
      qWarning() << "shortcut" << table[loser].sequence.toString(QKeySequence::PortableText)
                 << "claimed by both" << table[winner].key << "and" << table[loser].key
                 << "- unbinding" << table[loser].key;
      table[loser].sequence = QKeySequence();
      if (loser == i) {
        break;
      }
    }
  }

  return table;
}

const QVector<Shortcuts::Spec> &Shortcuts::specs() const {
  return _specs;
}

QKeySequence Shortcuts::sequenceFor(Action action) const {
  for (const auto &spec : _specs) {
    if (spec.action == action) {
      return spec.sequence;
    }
  }
  return QKeySequence();
}

void Shortcuts::applyOverrides(const QMap<QString, QString> &overrides) {
  // merge, not replace: keys unknown to this build must survive
  auto stored = global_conf.shortcuts();
  const auto &table = defaults();
  for (const auto &spec : table) {
    stored.remove(spec.key);
  }
  for (auto i = overrides.cbegin(); i != overrides.cend(); ++i) {
    stored.insert(i.key(), i.value());
  }

  _specs = resolve(stored);
  for (int i = 0; i < _specs.size() && i < _local.size(); i++) {
    if (_local[i] != nullptr) {
      _local[i]->setKey(_specs[i].sequence);
    }
  }

  global_conf.saveShortcuts(stored);
  global_conf.sync();

  emit changed();
}

void Shortcuts::setupGlobal() {
#if defined(ENABLE_QHOTKEY) && !defined(Q_OS_MACOS) && !defined(SMTC_ENABLE)
  if (QGuiApplication::platformName() == "wayland") {
    // wayland not supported and results in crash
    // https://github.com/olegantonyan/mpz/issues/129
    return;
  }
  if (local_conf.disableQhotkey()) {
    return;
  }
  // Skip where the OS owns the media keys — macOS (MPRemoteCommandCenter) and the SMTC
  // build (WindowsMediaControls); a QHotkey grab would double-fire alongside them.
#ifdef Q_OS_WIN
  connect(&_playpause_global, &QHotkey::activated, this, &Shortcuts::playPause);
#else
  connect(&_play_global, &QHotkey::activated, this, &Shortcuts::play);
  connect(&_pause_global, &QHotkey::activated, this, &Shortcuts::pause);
#endif
  connect(&_stop_global, &QHotkey::activated, this, &Shortcuts::stop);
  connect(&_prev_global, &QHotkey::activated, this, &Shortcuts::prev);
  connect(&_next_global, &QHotkey::activated, this, &Shortcuts::next);

  _stop_global.setShortcut(Qt::Key_MediaStop, Qt::NoModifier, true);
#ifdef Q_OS_WIN
  // Windows has no VK for a discrete pause — Key_MediaPause silently fails to register, and Key_MediaPlay is VK_MEDIA_PLAY_PAUSE, a single toggle key.
  _playpause_global.setShortcut(Qt::Key_MediaPlay, Qt::NoModifier, true);
#else
  _play_global.setShortcut(Qt::Key_MediaPlay, Qt::NoModifier, true);
  _pause_global.setShortcut(Qt::Key_MediaPause, Qt::NoModifier, true);
#endif
  _next_global.setShortcut(Qt::Key_MediaNext, Qt::NoModifier, true);
  _prev_global.setShortcut(Qt::Key_MediaPrevious, Qt::NoModifier, true);
#endif
}

void Shortcuts::setupLocal() {
  _local.reserve(_specs.size());
  for (const auto &spec : _specs) {
    if (!spec.registerLocal) {
      _local << nullptr;
      continue;
    }
    auto *sc = new QShortcut(spec.sequence, _parent);
    connect(sc, &QShortcut::activated, this, [this, action = spec.action]() { emitFor(action); });
    _local << sc;
  }
}

void Shortcuts::emitFor(Action action) {
  switch (action) {
    case Action::Quit: emit quit(); break;
    case Action::FocusLibrary: emit focusLibrary(); break;
    case Action::FocusPlaylists: emit focusPlaylists(); break;
    case Action::FocusPlaylist: emit focusPlaylist(); break;
    case Action::FocusFilterLibrary: emit focusFilterLibrary(); break;
    case Action::FocusFilterPlaylists: emit focusFilterPlaylists(); break;
    case Action::FocusFilterPlaylist: emit focusFilterPlaylist(); break;
    case Action::Play: emit play(); break;
    case Action::Pause: emit pause(); break;
    case Action::Stop: emit stop(); break;
    case Action::Prev: emit prev(); break;
    case Action::Next: emit next(); break;
    case Action::PlayPause: emit playPause(); break;
    case Action::VolumeUp: emit volumeUp(); break;
    case Action::VolumeDown: emit volumeDown(); break;
    case Action::Settings: emit openSettings(); break;
    case Action::OpenMainMenu: emit openMainMenu(); break;
    case Action::OpenPlaybackLog: emit openPlabackLog(); break;
    case Action::OpenSortMenu: emit openSortMenu(); break;
    case Action::OpenOutputMenu: emit openOutputMenu(); break;
    case Action::OpenShortcutsMenu: emit openShortcutsMenu(); break;
    case Action::JumpToPlayingTrack: emit jumpToPLayingTrack(); break;
  }
}
