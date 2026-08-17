#include "decode/nativeformat.h"

#include <audioproperties.h>
#include <fileref.h>
#include <flacproperties.h>

#include <QFileInfo>

namespace Decode {
  QAudioFormat nativeAudioFormat(const QString &path) {
    if (QFileInfo(path).suffix().compare(QStringLiteral("flac"), Qt::CaseInsensitive) != 0) {
      return QAudioFormat();
    }
    const TagLib::FileRef file(path.toUtf8().constData());
    if (file.isNull()) {
      return QAudioFormat();
    }
    const auto *props = dynamic_cast<TagLib::FLAC::Properties *>(file.audioProperties());
    if (props == nullptr || props->sampleRate() <= 0 || props->channels() <= 0 ||
        props->bitsPerSample() <= 0) {
      return QAudioFormat();
    }
    QAudioFormat format;
    format.setSampleRate(props->sampleRate());
    format.setChannelCount(props->channels());
    format.setChannelConfig(QAudioFormat::defaultChannelConfigForChannelCount(props->channels()));
    format.setSampleFormat(props->bitsPerSample() > 16 ? QAudioFormat::Int32 : QAudioFormat::Int16);
    return format;
  }
}
