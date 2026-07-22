#ifndef STREAMMETADATA_H
#define STREAMMETADATA_H

#include <QMap>

class StreamMetaData {
public:
  explicit StreamMetaData();
  explicit StreamMetaData(const QString &title);

  bool isEmpty() const;

  void insert(const QString& key, const QString &value);
  void setStatusNowPlaying(const QString &raw);
  void clear();

  quint16 bitrate() const;
  quint32 samplerate() const;

  QString artist() const;
  QString title() const;

  QString format() const;

private:
  QString nowPlaying() const;

  QMap<QString, QString> _data;

  QString title_explicit;
  QString _status_now_playing;
};

#endif // STREAMMETADATA_H
