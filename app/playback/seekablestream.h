#ifndef PLAYBACK_SEEKABLESTREAM_H
#define PLAYBACK_SEEKABLESTREAM_H

#include "playback/stream.h"

#include <QIODevice>
#include <QByteArray>
#include <QMutex>

namespace Playback {
  // Seekable view over a live Stream. Qt5's DirectShow backend, the only
  // QMediaPlayer backend in the MinGW Windows build, refuses sequential devices
  // outright and drives playback with positioned reads instead, so it gets a
  // sliding window of the most recent bytes to read from.
  class SeekableStream : public QIODevice {
    Q_OBJECT
  public:
    explicit SeekableStream(Stream &source, qint64 window_bytes = 16 * 1024 * 1024, QObject *parent = nullptr);

    void clear();

    // QIODevice interface
    bool isSequential() const override;
    bool atEnd() const override;
    qint64 pos() const override;
    qint64 size() const override;
    qint64 bytesAvailable() const override;
    bool seek(qint64 pos) override;

  public slots:
    void drain();

  protected:
    // QIODevice interface
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

  private:
    Stream &_source;
    const qint64 _window_bytes;
    mutable QMutex _mutex;
    QByteArray _window;
    qint64 _window_start; // absolute stream offset of _window[0]
    qint64 _read_pos;     // absolute stream offset
    bool _draining;
  };
}

#endif // PLAYBACK_SEEKABLESTREAM_H
