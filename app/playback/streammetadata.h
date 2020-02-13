#ifndef STREAMMETADATA_H
#define STREAMMETADATA_H

#include <QMap>

namespace Playback {
  class StreamMetaData {
  public:
    explicit StreamMetaData();

    void insert(const QString& key, const QString &value);
    void clear();
    QMap<QString, QString> rawData() const;

    quint16 bitrate() const;

  private:
    QMap<QString, QString> _data;
  };
}

#endif // STREAMMETADATA_H
