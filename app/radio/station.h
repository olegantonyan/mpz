#ifndef RADIO_STATION_H
#define RADIO_STATION_H

#include <QString>

namespace Radio {
  struct Station {
    QString id;
    QString name;
    QString group;
    QString url;
    QString codec;
    quint16 bitrate = 0;
    QString homepage;

    bool isValid() const;
    QString subtitle() const;
  };
}

#endif // RADIO_STATION_H
