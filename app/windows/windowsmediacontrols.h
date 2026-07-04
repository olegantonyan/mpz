#ifndef WINDOWSMEDIACONTROLS_H
#define WINDOWSMEDIACONTROLS_H

#include "playback/playbackcontroller.h"

#include <QObject>

class QWidget;

// Native Windows System Media Transport Controls (SMTC) integration: the media
// flyout / overlay that shows the current track + cover art and forwards
// play/pause/stop/next/prev and scrubbing. This is the Windows analogue of
// MacMediaControls (MPNowPlayingInfoCenter) and Mpris (D-Bus). MSVC-only; gated
// by the SMTC_ENABLE compile definition (see CMakeLists.txt) so it never reaches
// the MinGW Qt5 build. All WinRT/C++/WinRT usage is confined to the .cpp via the
// Impl struct, keeping the WinRT headers out of the rest of the tree.
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
