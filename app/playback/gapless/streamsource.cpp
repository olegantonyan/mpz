#include "playback/gapless/streamsource.h"

#include "playback/stream.h"

#include <QMutexLocker>

namespace Playback::Gapless {
  StreamSource::StreamSource(quint32 threshold_bytes, QObject *parent)
      : QIODevice(parent), threshold_bytes(threshold_bytes) {
    open(QIODevice::ReadOnly | QIODevice::Unbuffered);
  }

  StreamSource::~StreamSource() {
    shutdown();
  }

  void StreamSource::startStream(const QUrl &url) {
    Playback::Stream *old = nullptr;
    {
      QMutexLocker lock(&wait_mutex);
      old = stream;
      stream = nullptr;
      released.store(false);
      dead.store(false);
    }
    if (old) {
      old->disconnect(this);
      old->stop();
      old->deleteLater();
    }

    Playback::Stream *s = new Playback::Stream(threshold_bytes);
    connectStream(s);
    s->setUrl(url);
    fill_throttle.invalidate();
    {
      QMutexLocker lock(&wait_mutex);
      stream = s;
    }
    s->start();
  }

  void StreamSource::releaseRead() {
    QMutexLocker lock(&wait_mutex);
    released.store(true);
    wait_cond.wakeAll();
  }

  void StreamSource::rearm() {
    QMutexLocker lock(&wait_mutex);
    released.store(false);
  }

  void StreamSource::shutdown() {
    releaseRead();
    Playback::Stream *s = nullptr;
    {
      QMutexLocker lock(&wait_mutex);
      s = stream;
      stream = nullptr;
      dead.store(true);
    }
    if (s) {
      s->disconnect(this);
      s->stop();
      s->deleteLater();
    }
  }

  void StreamSource::connectStream(Playback::Stream *s) {
    // Wake lambdas fire on the network (and, for fillChanged, the demux) thread
    // with Stream's mutex released; they must do nothing but set flags and wake.
    connect(s, &Playback::Stream::fillChanged, this, [this](quint32, quint32) {
      QMutexLocker lock(&wait_mutex);
      wait_cond.wakeAll();
    }, Qt::DirectConnection);
    connect(s, &Playback::Stream::stopped, this, [this]() {
      QMutexLocker lock(&wait_mutex);
      dead.store(true);
      wait_cond.wakeAll();
    }, Qt::DirectConnection);
    connect(s, &Playback::Stream::error, this, [this](const QString &) {
      QMutexLocker lock(&wait_mutex);
      dead.store(true);
      wait_cond.wakeAll();
    }, Qt::DirectConnection);

    connect(s, &Playback::Stream::fillChanged, this, &StreamSource::forwardFill);
    // metadataChanged is emitted under Stream's mutex; forward auto/queued, never direct.
    connect(s, &Playback::Stream::metadataChanged, this, &StreamSource::metadataChanged);
    connect(s, &Playback::Stream::error, this, &StreamSource::streamError);
    connect(s, &Playback::Stream::stopped, this, &StreamSource::streamStopped);
  }

  void StreamSource::forwardFill(quint32 current, quint32 total) {
    // The threshold trigger (current >= total) must never be dropped by throttling.
    if (current < total && fill_throttle.isValid() && fill_throttle.elapsed() < 50) {
      return;
    }
    fill_throttle.restart();
    emit ringFillChanged(current, total);
  }

  bool StreamSource::isSequential() const {
    return true;
  }

  bool StreamSource::atEnd() const {
    if (released.load()) {
      return true;
    }
    if (!dead.load()) {
      return false;
    }
    QMutexLocker lock(&wait_mutex);
    return !stream || stream->bytesAvailable() == 0;
  }

  qint64 StreamSource::bytesAvailable() const {
    QMutexLocker lock(&wait_mutex);
    const qint64 ring = stream ? stream->bytesAvailable() : 0;
    return ring + QIODevice::bytesAvailable();
  }

  qint64 StreamSource::readData(char *data, qint64 maxlen) {
    // Runs on the decoder demux thread. The ffmpeg backend maps a 0-byte read or
    // atEnd() to AVERROR_EOF, so block until data, release, or dead+drained; the
    // 250ms timed wait is a safety net against a missed wake. Reads straight from
    // the ring at the decoder's rate — Engine back-pressure keeps the ring full.
    wait_mutex.lock();
    for (;;) {
      Playback::Stream *s = stream;
      wait_mutex.unlock();
      const qint64 n = s ? s->read(data, maxlen) : 0;
      wait_mutex.lock();
      if (n > 0) {
        wait_mutex.unlock();
        return n;
      }
      if (released.load()) {
        wait_mutex.unlock();
        return -1;
      }
      if (dead.load() && (!s || s->bytesAvailable() == 0)) {
        wait_mutex.unlock();
        return -1;
      }
      wait_cond.wait(&wait_mutex, 250);
    }
  }

  qint64 StreamSource::writeData(const char *data, qint64 len) {
    Q_UNUSED(data)
    Q_UNUSED(len)
    return -1;
  }
}
