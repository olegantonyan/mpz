#include "shortcuts.h"

#include <QGuiApplication>

Shortcuts::Shortcuts(QWidget *parent) : QObject(parent),
  _parent(parent),
  _play_global(parent),
  _pause_global(parent),
  _stop_global(parent),
  _prev_global(parent),
  _next_global(parent)
{
  setupLocal();
  setupGlobal();
}

const QVector<Shortcuts::Spec> &Shortcuts::specs() {
  // The one place every key sequence and dialog label lives. Each entry is
  // resolved for the current platform at first use. registerLocal is false for
  // actions the macOS native menu bar owns (Play/Pause, Stop, Prev, Next,
  // Volume, Settings, Quit) so we do not register a QShortcut that would fire
  // alongside the menu action.
  static const QVector<Spec> table = []() {
    QVector<Spec> t;
#ifdef Q_OS_MACOS
    t << Spec{Action::PlayPause, tr("Play / Pause"), QKeySequence(Qt::Key_Space), false};
    t << Spec{Action::Play, QString(), QKeySequence(), false};
    t << Spec{Action::Pause, QString(), QKeySequence(), false};
    t << Spec{Action::Stop, QString(), QKeySequence(), false};
    t << Spec{Action::Next, tr("Next"), QKeySequence(Qt::CTRL | Qt::Key_Right), false};
    t << Spec{Action::Prev, tr("Previous"), QKeySequence(Qt::CTRL | Qt::Key_Left), false};
    t << Spec{Action::VolumeUp, tr("Volume up"), QKeySequence(Qt::CTRL | Qt::Key_Up), false};
    t << Spec{Action::VolumeDown, tr("Volume down"), QKeySequence(Qt::CTRL | Qt::Key_Down), false};
    t << Spec{Action::Settings, tr("Settings"), QKeySequence(QKeySequence::Preferences), false};
    t << Spec{Action::FocusLibrary, tr("Focus on library"), QKeySequence(Qt::CTRL | Qt::Key_1), true};
    t << Spec{Action::FocusPlaylists, tr("Focus on playlists"), QKeySequence(Qt::CTRL | Qt::Key_2), true};
    t << Spec{Action::FocusPlaylist, tr("Focus on playlist"), QKeySequence(Qt::CTRL | Qt::Key_3), true};
    // Cmd+Option+digit, not Cmd+Shift+digit: the latter clashes with macOS
    // system screenshot shortcuts (⌘⇧3/⌘⇧4/⌘⇧5).
    t << Spec{Action::FocusFilterLibrary, tr("Focus on library filter"), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_1), true};
    t << Spec{Action::FocusFilterPlaylists, tr("Focus on playlists filter"), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_2), true};
    t << Spec{Action::FocusFilterPlaylist, tr("Focus on playlist filter"), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_3), true};
    t << Spec{Action::OpenMainMenu, tr("Open main menu"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_M), true};
    t << Spec{Action::OpenPlaybackLog, tr("Open playback log"), QKeySequence(Qt::CTRL | Qt::Key_L), true};
    t << Spec{Action::OpenSortMenu, tr("Open sort menu"), QKeySequence(Qt::CTRL | Qt::Key_S), true};
    t << Spec{Action::OpenShortcutsMenu, tr("Open shortcuts dialog"), QKeySequence(Qt::CTRL | Qt::Key_Slash), true};
    t << Spec{Action::JumpToPlayingTrack, tr("Jump to playing track"), QKeySequence(Qt::CTRL | Qt::Key_J), true};
    t << Spec{Action::Quit, tr("Quit"), QKeySequence(QKeySequence::Quit), false};
#else
    t << Spec{Action::PlayPause, tr("Play / Pause"), QKeySequence(Qt::Key_Space), true};
    t << Spec{Action::Play, tr("Play"), QKeySequence(Qt::ALT | Qt::Key_E), true};
    t << Spec{Action::Pause, tr("Pause"), QKeySequence(Qt::ALT | Qt::Key_W), true};
    t << Spec{Action::Stop, tr("Stop"), QKeySequence(Qt::ALT | Qt::Key_Q), true};
    t << Spec{Action::Next, tr("Next"), QKeySequence(Qt::ALT | Qt::Key_T), true};
    t << Spec{Action::Prev, tr("Previous"), QKeySequence(Qt::ALT | Qt::Key_R), true};
    t << Spec{Action::VolumeUp, tr("Volume up"), QKeySequence(Qt::CTRL | Qt::Key_Up), true};
    t << Spec{Action::VolumeDown, tr("Volume down"), QKeySequence(Qt::CTRL | Qt::Key_Down), true};
    t << Spec{Action::Settings, tr("Settings"), QKeySequence(Qt::CTRL | Qt::Key_Comma), true};
    t << Spec{Action::FocusLibrary, tr("Focus on library"), QKeySequence(Qt::CTRL | Qt::Key_1), true};
    t << Spec{Action::FocusPlaylists, tr("Focus on playlists"), QKeySequence(Qt::CTRL | Qt::Key_2), true};
    t << Spec{Action::FocusPlaylist, tr("Focus on playlist"), QKeySequence(Qt::CTRL | Qt::Key_3), true};
    t << Spec{Action::FocusFilterLibrary, tr("Focus on library filter"), QKeySequence(Qt::ALT | Qt::Key_1), true};
    t << Spec{Action::FocusFilterPlaylists, tr("Focus on playlists filter"), QKeySequence(Qt::ALT | Qt::Key_2), true};
    t << Spec{Action::FocusFilterPlaylist, tr("Focus on playlist filter"), QKeySequence(Qt::ALT | Qt::Key_3), true};
    t << Spec{Action::OpenMainMenu, tr("Open main menu"), QKeySequence(Qt::ALT | Qt::Key_M), true};
    t << Spec{Action::OpenPlaybackLog, tr("Open playback log"), QKeySequence(Qt::CTRL | Qt::Key_L), true};
    t << Spec{Action::OpenSortMenu, tr("Open sort menu"), QKeySequence(Qt::CTRL | Qt::Key_S), true};
    t << Spec{Action::OpenShortcutsMenu, tr("Open shortcuts dialog"), QKeySequence(Qt::ALT | Qt::Key_S), true};
    t << Spec{Action::JumpToPlayingTrack, tr("Jump to playing track"), QKeySequence(Qt::ALT | Qt::Key_J), true};
    t << Spec{Action::Quit, tr("Quit"), QKeySequence(Qt::CTRL | Qt::Key_Q), true};
#endif
    return t;
  }();
  return table;
}

QKeySequence Shortcuts::sequenceFor(Action action) {
  for (const auto &spec : specs()) {
    if (spec.action == action) {
      return spec.sequence;
    }
  }
  return QKeySequence();
}

QVector<QPair<QString, QString> > Shortcuts::describe() const {
  QVector<QPair<QString, QString> > r;
  for (const auto &spec : specs()) {
    if (spec.description.isEmpty()) {
      continue;
    }
    r << QPair<QString, QString>(spec.description, spec.sequence.toString(QKeySequence::NativeText));
  }
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
  for (const auto &spec : specs()) {
    if (!spec.registerLocal || spec.sequence.isEmpty()) {
      continue;
    }
    auto *sc = new QShortcut(spec.sequence, _parent);
    connect(sc, &QShortcut::activated, this, [this, action = spec.action]() { emitFor(action); });
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
    case Action::OpenShortcutsMenu: emit openShortcutsMenu(); break;
    case Action::JumpToPlayingTrack: emit jumpToPLayingTrack(); break;
  }
}
