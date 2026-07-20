#ifndef GAPLESS_STREAMSOURCE_H
#define GAPLESS_STREAMSOURCE_H

#include "streammetadata.h"

#include <QElapsedTimer>
#include <QIODevice>
#include <QMutex>
#include <QUrl>
#include <QWaitCondition>

#include <atomic>

namespace Playback {
  class Stream;
}

namespace Playback::Gapless {
  class StreamSource : public QIODevice {
    Q_OBJECT
  public:
    explicit StreamSource(quint32 threshold_bytes, QObject *parent = nullptr);
    ~StreamSource() override;

    void startStream(const QUrl &url);
    void releaseRead();
    void rearm();
    void shutdown();

    bool isSequential() const override;
    bool atEnd() const override;
    qint64 bytesAvailable() const override;

  signals:
    void ringFillChanged(quint32 current, quint32 total);
    void metadataChanged(const StreamMetaData &meta);
    void streamError(const QString &message);
    void streamStopped();

  protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

  private:
    void connectStream(Playback::Stream *s);
    void forwardFill(quint32 current, quint32 total);

    const quint32 threshold_bytes; // ring size for the wrapped Stream
    Playback::Stream *stream = nullptr;
    mutable QMutex wait_mutex;
    QWaitCondition wait_cond;
    std::atomic<bool> released{false};
    std::atomic<bool> dead{false};
    QElapsedTimer fill_throttle; // GUI thread only
  };
}

#endif // GAPLESS_STREAMSOURCE_H
