#ifndef TRACK_H
#define TRACK_H

#include "streammetadata.h"

#include <QString>
#include <QUrl>

class Track {
public:
  explicit Track();
  explicit Track(const QString &filepath, quint32 begin = 0);
  explicit Track(const QString &filepath,
                 quint32 begin,
                 const QString &artist,
                 const QString &album,
                 const QString &title,
                 quint16 tracknum,
                 quint16 year,
                 quint32 duration,
                 quint8 channels,
                 quint16 bitrate,
                 quint16 samplerate);
  explicit Track(const QUrl &stream_url, const QString &filepath_reference);

  static QString formattedTime(quint32 tm);

  bool isValid() const;

  bool fillAudioProperties();
  bool fillTags();
  bool reload();
  void setDuration(quint32 dur);
  void setCue(bool is_cue = true);

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
  quint32 begin() const;

  bool isCue() const;
  QString formattedDuration() const;
  QString formattedAudioInfo() const;
  QString shortText() const;

  quint64 uid() const;

  QString dir() const;

  QString formattedTitle() const;

  bool isStream() const;

  void setStreamMeta(const StreamMetaData& meta);
  void clearStreamMeta();

  const StreamMetaData &streamMeta() const;

  QString artCover() const;

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
  QUrl _stream_url;
  quint32 _begin;

  quint64 _uid;

  bool _cue;

  StreamMetaData _stream_meta;

  quint64 generateUid() const;
  QString detectFormat() const;
};

#endif // TRACK_H
