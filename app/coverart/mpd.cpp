#include "coverart/mpd.h"

#include <QStandardPaths>
#include <QDir>

namespace CoverArt {
  Mpd::Mpd(MpdClient::Client &cl) : client(cl) {
    temp_dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QDir::separator() + "mpd_covers" + QDir::separator();
    if (!QDir(temp_dir).exists()) {
      QDir().mkdir(temp_dir);
    }
  }

  QString Mpd::get(const QString &filepath) {
    auto binary = client.albumArt(filepath);
    if (!binary.isEmpty()) {

    }
    return "";
  }
}
