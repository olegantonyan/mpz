#ifndef DYNAMIC_RANGE_LOGFORMAT_H
#define DYNAMIC_RANGE_LOGFORMAT_H

#include "dynamic_range/dr.h"

#include <QDateTime>
#include <QString>
#include <QVector>

namespace DynamicRange {
  struct Entry {
    QString artist;
    QString album;
    QString display;
    quint32 duration_ms = 0;
    quint32 sample_rate = 0;
    quint8 channels = 0;
    quint16 bits_per_sample = 0;
    quint16 bitrate = 0;
    QString codec;
    Result result;
  };

  struct LogMeta {
    QString app_version;
    QDateTime when;
  };

  QString formatLog(const QVector<Entry> &entries, const LogMeta &meta);
}

#endif // DYNAMIC_RANGE_LOGFORMAT_H
