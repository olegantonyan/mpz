#include "decode/nativeformat.h"

#include "track.h"

#include <QFileInfo>

namespace Decode {
  QAudioFormat nativeAudioFormat(const QString &path) {
    if (QFileInfo(path).suffix().compare(QStringLiteral("flac"), Qt::CaseInsensitive) != 0) {
      return QAudioFormat();
    }
    const Track::AudioProperties props = Track::audioPropertiesOf(path);
    if (props.sample_rate == 0 || props.channels == 0) {
      return QAudioFormat();
    }
    QAudioFormat format;
    format.setSampleRate(static_cast<int>(props.sample_rate));
    format.setChannelCount(static_cast<int>(props.channels));
    format.setChannelConfig(QAudioFormat::defaultChannelConfigForChannelCount(props.channels));
    format.setSampleFormat(props.bits_per_sample > 16 ? QAudioFormat::Int32 : QAudioFormat::Int16);
    return format;
  }
}
