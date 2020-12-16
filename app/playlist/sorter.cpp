#include "sorter.h"

namespace Playlist {
  Sorter::Sorter(const QString &c) {
    for (auto i : c.split("/")) {
      if (!i.isEmpty()) {
        criteria << i.simplified().toUpper();
      }
    }
    //qDebug() << criteria;
  }

  QString Sorter::defaultCriteria() {
    return "YEAR / ALBUM / DIRECTORY / TRACKNUMBER / FILENAME / TITLE";
  }

  bool Sorter::condition(const Track &t1, const Track &t2) const {
    for (auto i : criteria) {
      int cmp = compare(t1, t2, i);
      if (cmp > 0) {
        return true;
      } else if (cmp < 0) {
        return false;
      }
    }

    return false;
  }

  int Sorter::compare(const Track &t1, const Track &t2, QString attr) const {
    int result = 0;
    int order = 1;

    if (attr.startsWith("-")) {
      attr.remove(0, 1);
      order = -1;
    }

    if (attr == "ARTIST") {
      result = compare_artist(t1, t2);
    } else if (attr == "ALBUM") {
      result = compare_album(t1, t2);
    } else if (attr == "YEAR") {
      result = compare_year(t1, t2);
    } else if (attr == "TRACKNUMBER") {
      result = compare_track_number(t1, t2);
    } else if (attr == "FILENAME") {
      result = compare_filename(t1, t2);
    } else if (attr == "TITLE") {
      result = compare_title(t1, t2);
    }else if (attr == "DIRECTORY") {
      result = compare_dir(t1, t2);
    }

    return result * order;
  }

  int Sorter::compare_year(const Track &t1, const Track &t2) const {
    if (t1.year() < t2.year()) {
      return 1;
    } else if (t1.year() > t2.year()) {
      return -1;
    }
    return 0;
  }

  int Sorter::compare_album(const Track &t1, const Track &t2) const {
    if (t1.album() < t2.album()) {
      return 1;
    } else if (t1.album() > t2.album()) {
      return -1;
    }
    return 0;
  }

  int Sorter::compare_track_number(const Track &t1, const Track &t2) const {
    if (t1.track_number() < t2.track_number()) {
      return 1;
    } else if (t1.track_number() > t2.track_number()) {
      return -1;
    }
    return 0;
  }

  int Sorter::compare_filename(const Track &t1, const Track &t2) const {
    if (t1.filename() < t2.filename()) {
      return 1;
    } else if (t1.filename() > t2.filename()) {
      return -1;
    }
    return 0;
  }

  int Sorter::compare_title(const Track &t1, const Track &t2) const {
    if (t1.title() < t2.title()) {
      return 1;
    } else if (t1.title() > t2.title()) {
      return -1;
    }
    return 0;
  }

  int Sorter::compare_artist(const Track &t1, const Track &t2) const {
    if (t1.artist() < t2.artist()) {
      return 1;
    } else if (t1.artist() > t2.artist()) {
      return -1;
    }
    return 0;
  }

  int Sorter::compare_dir(const Track &t1, const Track &t2) const {
    if (t1.dir() < t2.dir()) {
      return 1;
    } else if (t1.dir() > t2.dir()) {
      return -1;
    }
    return 0;
  }
}
