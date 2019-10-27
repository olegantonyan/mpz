#include "trackwrapper.h"

TrackWrapper::TrackWrapper() {
  TrackWrapper(Track(), 0, 0);
}

TrackWrapper::TrackWrapper(const Track &t, int t_index, int p_index) : track(t), playlist_index(p_index), track_index(t_index) {
}
