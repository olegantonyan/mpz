#include "radio/station.h"

#include <QUrl>

namespace Radio {
  bool Station::isValid() const {
    if (id.isEmpty() || name.isEmpty()) {
      return false;
    }
    const QUrl parsed(url);
    return parsed.isValid() && (parsed.scheme() == "http" || parsed.scheme() == "https");
  }

  QString Station::subtitle() const {
    if (codec.isEmpty()) {
      return QString();
    }
    return bitrate > 0 ? QString("%1 %2k").arg(codec.toUpper()).arg(bitrate) : codec.toUpper();
  }
}
