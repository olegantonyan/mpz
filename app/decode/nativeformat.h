#ifndef DECODE_NATIVEFORMAT_H
#define DECODE_NATIVEFORMAT_H

#include <QAudioFormat>
#include <QString>

namespace Decode {
  // Invalid unless FLAC: Qt derives a 0 Hz format for frame headers that defer the
  // rate to STREAMINFO and then decodes nothing. Invalid means Qt's own derivation.
  QAudioFormat nativeAudioFormat(const QString &path);
}

#endif // DECODE_NATIVEFORMAT_H
