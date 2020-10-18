#include "sorter.h"


namespace Playlist {
  Sorter::Sorter(const QString &c) : criteria(c) {
  }

  bool Sorter::condition(const Track &t1, const Track &t2) const {
    // %ARTIST% - %DATE% - %ALBUM% - %DISCNUMBER% - %TRACKNUMBER% - %TITLE%

    /*if (t1.artist() < t2.artist()) {
      return true;
    } else if (t1.artist() > t2.artist()) {
      return false;
    }*/

    if (t1.year() < t2.year()) {
      return true;
    } else if (t1.year() > t2.year()) {
      return false;
    }

    if (t1.album() < t2.album()) {
      return true;
    } else if (t1.album() > t2.album()) {
      return false;
    }

    if (t1.track_number() < t2.track_number()) {
      return true;
    } else if (t1.track_number() > t2.track_number()) {
      return false;
    }

    if (t1.filename() < t2.filename()) {
      return true;
    } else if (t1.filename() > t2.filename()) {
      return false;
    }

    if (t1.title() < t2.title()) {
      return true;
    } else if (t1.title() > t2.title()) {
      return false;
    }

    return false;
  }
}
