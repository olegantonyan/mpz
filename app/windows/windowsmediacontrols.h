#ifndef WINDOWSMEDIACONTROLS_H
#define WINDOWSMEDIACONTROLS_H

#include "playback/playbackcontroller.h"

#include <QObject>

class QWidget;

// Windows SMTC integration (analogue of MacMediaControls / Mpris). WinRT is kept
// in the .cpp via Impl so its headers don't leak into the rest of the tree.
class WindowsMediaControls : public QObject {
  Q_OBJECT
public:
  explicit WindowsMediaControls(Playback::Controller *player, QWidget *window, QObject *parent = nullptr);
  ~WindowsMediaControls() override;

private:
  void updateMetadata();
  void updateState(Playback::Controller::State state);
  void updateTimeline();

  Playback::Controller *player;

  struct Impl;
  Impl *d;
};

#endif // WINDOWSMEDIACONTROLS_H
