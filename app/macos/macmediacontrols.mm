#include "macmediacontrols.h"

#include "track.h"

#import <MediaPlayer/MediaPlayer.h>
#import <AppKit/AppKit.h>

MacMediaControls::MacMediaControls(Playback::Controller *pl, QObject *parent) : QObject(parent), player(pl) {
  setupRemoteCommands();

  connect(player, &Playback::Controller::trackChanged, this, [this](const Track &) {
    updateNowPlayingInfo();
  });
  connect(player, &Playback::Controller::started, this, [this](const Track &) {
    updateNowPlayingInfo();
    updatePlaybackState(Playback::Controller::Playing);
  });
  connect(player, &Playback::Controller::paused, this, [this](const Track &) {
    updateNowPlayingInfo();
    updatePlaybackState(Playback::Controller::Paused);
  });
  connect(player, &Playback::Controller::stopped, this, [this]() {
    [MPNowPlayingInfoCenter defaultCenter].nowPlayingInfo = @{};
    updatePlaybackState(Playback::Controller::Stopped);
  });
  connect(player, &Playback::Controller::seeked, this, [this](int) {
    updateNowPlayingInfo();
  });
}

MacMediaControls::~MacMediaControls() {
  [MPNowPlayingInfoCenter defaultCenter].nowPlayingInfo = @{};
  [MPNowPlayingInfoCenter defaultCenter].playbackState = MPNowPlayingPlaybackStateStopped;
}

void MacMediaControls::updateNowPlayingInfo() {
  const Track &track = player->currentTrack();
  const bool is_stream = track.isStream();

  NSMutableDictionary *info = [NSMutableDictionary dictionary];
  info[MPMediaItemPropertyTitle] = track.title().toNSString();
  info[MPMediaItemPropertyArtist] = track.artist().toNSString();
  info[MPMediaItemPropertyAlbumTitle] = track.album().toNSString();
  info[MPMediaItemPropertyAlbumTrackNumber] = @(track.track_number());
  info[MPNowPlayingInfoPropertyElapsedPlaybackTime] = @((double)player->position());
  info[MPNowPlayingInfoPropertyPlaybackRate] = @(player->state() == Playback::Controller::Playing ? 1.0 : 0.0);

  if (!is_stream) {
    info[MPMediaItemPropertyPlaybackDuration] = @(track.duration() / 1000.0);
  }

  const QString art = track.artCover();
  if (!art.isEmpty()) {
    NSImage *image = [[NSImage alloc] initWithContentsOfFile:art.toNSString()];
    if (image != nil) {
      MPMediaItemArtwork *artwork = [[MPMediaItemArtwork alloc] initWithBoundsSize:image.size
                                                                    requestHandler:^NSImage *(CGSize) { return image; }];
      info[MPMediaItemPropertyArtwork] = artwork;
    }
  }

  [MPNowPlayingInfoCenter defaultCenter].nowPlayingInfo = info;

  [MPRemoteCommandCenter sharedCommandCenter].changePlaybackPositionCommand.enabled = !is_stream;
}

void MacMediaControls::updatePlaybackState(Playback::Controller::State state) {
  MPNowPlayingPlaybackState mac_state = MPNowPlayingPlaybackStateStopped;
  switch (state) {
    case Playback::Controller::Playing: mac_state = MPNowPlayingPlaybackStatePlaying; break;
    case Playback::Controller::Paused:  mac_state = MPNowPlayingPlaybackStatePaused; break;
    case Playback::Controller::Stopped: mac_state = MPNowPlayingPlaybackStateStopped; break;
  }
  [MPNowPlayingInfoCenter defaultCenter].playbackState = mac_state;
}

void MacMediaControls::setupRemoteCommands() {
  MPRemoteCommandCenter *center = [MPRemoteCommandCenter sharedCommandCenter];
  Playback::Controller *pl = player;

  center.playCommand.enabled = YES;
  [center.playCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *) {
    pl->controls().play->click();
    return MPRemoteCommandHandlerStatusSuccess;
  }];

  center.pauseCommand.enabled = YES;
  [center.pauseCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *) {
    pl->controls().pause->click();
    return MPRemoteCommandHandlerStatusSuccess;
  }];

  center.togglePlayPauseCommand.enabled = YES;
  [center.togglePlayPauseCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *) {
    if (pl->isStopped()) {
      pl->controls().play->click();
    } else {
      pl->controls().pause->click();
    }
    return MPRemoteCommandHandlerStatusSuccess;
  }];

  center.stopCommand.enabled = YES;
  [center.stopCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *) {
    pl->controls().stop->click();
    return MPRemoteCommandHandlerStatusSuccess;
  }];

  center.nextTrackCommand.enabled = YES;
  [center.nextTrackCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *) {
    pl->controls().next->click();
    return MPRemoteCommandHandlerStatusSuccess;
  }];

  center.previousTrackCommand.enabled = YES;
  [center.previousTrackCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *) {
    pl->controls().prev->click();
    return MPRemoteCommandHandlerStatusSuccess;
  }];

  center.changePlaybackPositionCommand.enabled = YES;
  [center.changePlaybackPositionCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *event) {
    MPChangePlaybackPositionCommandEvent *e = (MPChangePlaybackPositionCommandEvent *)event;
    pl->seek((int)e.positionTime);
    return MPRemoteCommandHandlerStatusSuccess;
  }];
}
