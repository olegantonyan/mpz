#ifndef MACMEDIACONTROLS_H
#define MACMEDIACONTROLS_H

#include "playback/playbackcontroller.h"

#include <QObject>

class MacMediaControls : public QObject {
  Q_OBJECT
public:
  explicit MacMediaControls(Playback::Controller *player, QObject *parent = nullptr);
  ~MacMediaControls() override;

private:
  void updateNowPlayingInfo();
  void updatePlaybackState(Playback::Controller::State state);
  void setupRemoteCommands();

  Playback::Controller *player;
};

#endif // MACMEDIACONTROLS_H
