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
    QString quality;
    if (!codec.isEmpty()) {
      quality = bitrate > 0 ? QString("%1 %2k").arg(codec.toUpper()).arg(bitrate)
                            : codec.toUpper();
    }
    if (quality.isEmpty()) {
      return description;
    }
    if (description.isEmpty()) {
      return quality;
    }
    return quality + QStringLiteral(" · ") + description;
  }
}
