#ifndef TRACK_H
#define TRACK_H

#include <QString>

class Track {
public:
  Track(const QString &filepath);

  QString path();
  QString artist();
  QString album();
  QString title();
  quint16 year();

private:
  QString filepath;
  QString _artist;
  QString _album;
  QString _title;
  quint16 _year;
};

#endif // TRACK_H
