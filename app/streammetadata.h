#ifndef STREAMMETADATA_H
#define STREAMMETADATA_H

#include <QMap>

class StreamMetaData {
public:
  explicit StreamMetaData();

  bool isEmpty() const;

  void insert(const QString& key, const QString &value);
  void clear();

  quint16 bitrate() const;
  quint16 samplerate() const;

  QString artist() const;
  QString title() const;

  QString format() const;

private:
  QMap<QString, QString> _data;
};


#endif // STREAMMETADATA_H
