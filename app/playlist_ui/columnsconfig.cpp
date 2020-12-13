#include "columnsconfig.h"

namespace PlaylistUi {
  ColumnsConfig::ColumnsConfig() {
  }

  int ColumnsConfig::count() const {
    return 5;
  }

  double ColumnsConfig::width(int col) const {
    if (col == 5) {
      return 0;
    } else if (col == 4) {
      return 0.05;
    }
    return 0.28;
  }

  bool ColumnsConfig::stretch(int col) const {
    if (col == 5) {
      return true;
    }
    return false;
  }

  QString ColumnsConfig::field(int col) const {
    switch (col) {
      case 1:
        return "artist";
      case 2:
        return "album";
      case 3:
        return "title";
      case 4:
        return "year";
      case 5:
        return "length";
    }
    return "";
  }

  Qt::Alignment ColumnsConfig::align(int col) const {
    if (col == 4 || col == 5) {
      return (Qt::AlignRight | Qt::AlignVCenter);
    } else {
      return Qt::AlignVCenter;
    }
  }

  QString ColumnsConfig::value(int col, const Track &track) const {
    auto fld = field(col);
    if (fld == "artist") {
      return track.artist();
    } else if (fld == "album") {
      return track.album();
    } else if (fld == "title") {
      return track.title();
    } else if (fld == "year" && track.year() > 0) {
      return QString::number(track.year());
    } else if (fld == "length") {
      return track.formattedDuration();
    }
    return QString();
  }
}
