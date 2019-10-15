#ifndef TRACK_H
#define TRACK_H

#include <QString>

class Track {
public:
  Track(const QString &filepath);

  QString path() const;
  QString artist() const;
  QString album() const;
  QString title() const;
  quint16 year() const;
  quint32 duration() const;

  QString formattedDuration() const;

private:
  QString filepath;
  QString _artist;
  QString _album;
  QString _title;
  quint16 _year;
  quint32 _duration;
};

#endif // TRACK_H
