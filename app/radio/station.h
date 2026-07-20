#ifndef RADIO_STATION_H
#define RADIO_STATION_H

#include <QString>

namespace Radio {
  struct Station {
    QString id;
    QString name;
    QString group;
    QString description;
    QString url;
    QString codec;
    quint16 bitrate = 0;
    QString homepage;
    QString logo_url;

    bool isValid() const;
    QString subtitle() const;   // "MP3 256k · description"
  };
}

#endif // RADIO_STATION_H
