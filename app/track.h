#ifndef TRACK_H
#define TRACK_H

#include <QString>
#include <QUrl>

class Track {
public:
  explicit Track();
  explicit Track(const QString &filepath);
  explicit Track(const QString &filepath,
                 const QString &artist,
                 const QString &album,
                 const QString &title,
                 quint16 tracknum,
                 quint16 year,
                 quint32 duration,
                 quint8 channels,
                 quint16 bitrate,
                 quint16 samplerate);

  static QString formattedTime(quint32 tm);

  bool isValid() const;

  QString path() const;
  QUrl url() const;
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

  QString formattedTitle() const;

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

  quint64 generateUid() const;
  QString detectFormat() const;
};

#endif // TRACK_H
