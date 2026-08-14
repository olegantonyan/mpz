#ifndef SHORTCUTS_H
#define SHORTCUTS_H

#include "config/local.h"
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
#include <QMap>
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

  // empty description hides it from the dialog; registerLocal false means the macOS menu bar owns the key
  struct Spec {
    Action action;
    QString key;
    QString description;
    QKeySequence sequence;
    bool registerLocal;
  };

  explicit Shortcuts(Config::Global &global_c, Config::Local &local_c, QWidget *parent);

  static const QVector<Spec> &defaults();
  static QVector<Spec> resolve(const QMap<QString, QString> &overrides);

  const QVector<Spec> &specs() const;
  QKeySequence sequenceFor(Action action) const;

  void applyOverrides(const QMap<QString, QString> &overrides);

signals:
  void changed();

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
  void openEqualizer();
  void openReplayGain();
  void openMainMenu();
  void openPlabackLog();
  void openSortMenu();
  void openOutputMenu();
  void openShortcutsMenu();
  void jumpToPLayingTrack();

private:
  void setupGlobal();
  void setupLocal();
  void emitFor(Action action);

  QWidget *_parent;
  Config::Global &global_conf;
  Config::Local &local_conf;

  QVector<Spec> _specs;
  // index-aligned with _specs, null where registerLocal is false
  QVector<QShortcut *> _local;

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
