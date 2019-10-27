#ifndef TRACKWRAPPER_H
#define TRACKWRAPPER_H

#include "track.h"

class TrackWrapper {
public:
  TrackWrapper();
  TrackWrapper(const Track &t, int t_index, int p_index);

  Track track;
  int playlist_index;
  int track_index;
};

#endif // TRACKWRAPPER_H
