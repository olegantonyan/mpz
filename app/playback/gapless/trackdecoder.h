#ifndef GAPLESS_TRACKDECODER_H
#define GAPLESS_TRACKDECODER_H

#include <QAudioDecoder>
#include <QAudioFormat>
#include <QByteArray>
#include <QObject>
#include <QUrl>

namespace Playback::Gapless {
  class TrackDecoder : public QObject {
    Q_OBJECT
  public:
    explicit TrackDecoder(QObject *parent = nullptr);

    void start(const QUrl &url);
    void stop();
    void requestFormat(const QAudioFormat &format);
    void setPaused(bool p);
    void pump();

  signals:
    void firstFormat(const QAudioFormat &format);
    void pcm(const QUrl &url, const QByteArray &interleaved_bytes);
    void finished(const QUrl &url, qint64 total_frames_decoded);
    void decodeError(const QUrl &url, const QString &message);

  private:
    void emitFormatOnce(const QAudioFormat &format);

    QAudioDecoder decoder;
    QUrl current_url;
    QAudioFormat requested_format;
    bool format_emitted = false;
    bool paused = false;
    qint64 total_frames = 0;
  };
}

#endif // GAPLESS_TRACKDECODER_H
