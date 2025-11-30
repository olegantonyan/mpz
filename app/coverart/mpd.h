#ifndef COVERART_MPD_H
#define COVERART_MPD_H

#include "mpd_client/client.h"

#include <QString>

namespace CoverArt {
  class Mpd {
  public:
    Mpd(MpdClient::Client &cl);

    QString get(const QString &filepath);

  private:
    QString temp_dir;

    MpdClient::Client &client;
  };
}

#endif // COVERART_MPD_H
