#ifndef SHORTCUTS_H
#define SHORTCUTS_H

#include "config/global.h"

#ifdef ENABLE_QHOTKEY
#include <qhotkey.h>
#endif

#include <QObject>
#include <QWidget>
#include <QtGlobal>
#include <QShortcut>
#include <QKeySequence>
#include <QVector>
#include <QPair>
#include <QString>

class Shortcuts : public QObject {
  Q_OBJECT
public:
  enum class Action {
    Quit,
    FocusLibrary,
    FocusPlaylists,
    FocusPlaylist,
    FocusFilterLibrary,
    FocusFilterPlaylists,
    FocusFilterPlaylist,
    Play,
    Pause,
    Stop,
    Prev,
    Next,
    PlayPause,
    VolumeUp,
    VolumeDown,
    Settings,
    OpenMainMenu,
    OpenPlaybackLog,
    OpenSortMenu,
    OpenOutputMenu,
    OpenShortcutsMenu,
    JumpToPlayingTrack
  };

  // Single source of truth for one shortcut: its action, the dialog label
  // (empty when it should not appear in the shortcuts dialog), the platform-
  // resolved key sequence, and whether a QShortcut should be registered for it
  // (false when the macOS native menu bar already owns the key, to avoid
  // double-firing).
  struct Spec {
    Action action;
    QString description;
    QKeySequence sequence;
    bool registerLocal;
  };

  explicit Shortcuts(QWidget *parent);

  QVector<QPair<QString, QString>> describe() const;

  // The canonical, platform-resolved key for an action. The macOS menu bar
  // reads its shortcuts from here so the menu and the dialog never drift.
  static QKeySequence sequenceFor(Action action);

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
  void playPause();
  void volumeUp();
  void volumeDown();
  void openSettings();
  void openMainMenu();
  void openPlabackLog();
  void openSortMenu();
  void openOutputMenu();
  void openShortcutsMenu();
  void jumpToPLayingTrack();

private:
  static const QVector<Spec> &specs();
  void setupGlobal();
  void setupLocal();
  void emitFor(Action action);

  QWidget *_parent;

#ifdef ENABLE_QHOTKEY
#ifdef Q_OS_WIN
  QHotkey _playpause_global;
#else
  QHotkey _play_global;
  QHotkey _pause_global;
#endif
  QHotkey _stop_global;
  QHotkey _prev_global;
  QHotkey _next_global;
#endif
};

#endif // SHORTCUTS_H
