#include "streammetadata.h"

#include <QDebug>
#include <QRegularExpression>

StreamMetaData::StreamMetaData() {
}

bool StreamMetaData::isEmpty() const {
  return _data.isEmpty();
}

void StreamMetaData::insert(const QString &key, const QString &value) {
  _data.insert(key, value);
}

void StreamMetaData::clear() {
  _data.clear();
}

quint16 StreamMetaData::bitrate() const {
  bool ok = false;
  auto i = _data.value("icy-br", "0").toUInt(&ok);
  if (!ok) {
    return 0;
  }
  return static_cast<quint16>(i);
}

quint16 StreamMetaData::samplerate() const {
  bool ok = false;
  auto i = _data.value("icy-sr", "0").toUInt(&ok);
  if (!ok) {
    return 0;
  }
  return static_cast<quint16>(i);
}

QString StreamMetaData::artist() const {
  QRegularExpression r("StreamTitle=('|\")(.*?)('|\");");
  auto stream = _data.value("stream", "");
  auto match = r.match(stream);
  if (match.hasMatch()) {
    return match.captured(2).split(" - ").first();
  }
  return "";
}

QString StreamMetaData::title() const {
  QRegularExpression r("StreamTitle=('|\")(.*?)('|\");");
  auto stream = _data.value("stream", "");
  auto match = r.match(stream);
  if (match.hasMatch()) {
    return match.captured(2).split(" - ").last();
  }
  return "";
}

QString StreamMetaData::format() const {
  return _data.value("content-type", "");
}
