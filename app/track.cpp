#include "track.h"

#include "fileref.h"
#include "tag.h"
#include "tpropertymap.h"

Track::Track(const QString &fp) {
  filepath = fp;

  TagLib::FileRef f(path().toStdString().c_str());
  if(!f.isNull() && f.tag()) {
    TagLib::Tag *tag = f.tag();
    _artist = QString(tag->artist().toCString());
    _album = QString(tag->album().toCString());
    _title = QString(tag->title().toCString());
    _year = static_cast<quint16>(tag->year());
  }
}

QString Track::path() {
  return filepath;
}

QString Track::artist() {
  return _artist;
}

QString Track::album() {
  return _album;
}

QString Track::title() {
  return _title;
}

quint16 Track::year() {
  return _year;
}
