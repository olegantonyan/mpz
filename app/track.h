#ifndef TRACK_H
#define TRACK_H

#include <QString>

class Track {
public:
  explicit Track();
  explicit Track(const QString &filepath);

  static QString formattedTime(quint32 tm);

  bool isValid() const;

  QString path() const;
  QString artist() const;
  QString album() const;
  QString title() const;
  quint16 year() const;
  quint32 duration() const;
  quint16 sample_rate() const;
  quint8 channels() const;
  quint16 bitrate() const;
  QString format() const;
  QString filename() const;
  quint16 track_number() const;

  QString formattedDuration() const;
  QString formattedAudioInfo() const;

  quint64 uid() const;

  QString dir() const;

private:
  QString filepath;
  QString _artist;
  QString _album;
  QString _title;
  quint16 _year;
  quint32 _duration;
  quint16 _sample_rate;
  quint8 _channels;
  quint16 _bitrate;
  QString _format;
  quint16 _track_number;

  quint64 _uid;
};

#endif // TRACK_H
