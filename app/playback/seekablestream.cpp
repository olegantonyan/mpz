#include "seekablestream.h"

#include <QMutexLocker>

#include <cstring>

namespace Playback {
  SeekableStream::SeekableStream(Stream &source, qint64 window_bytes, QObject *parent) :
    QIODevice(parent),
    _source(source),
    _window_bytes(window_bytes),
    _window_start(0),
    _read_pos(0),
    _draining(false) {
    open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    connect(&_source, &Stream::fillChanged, this, &SeekableStream::drain);
  }

  void SeekableStream::clear() {
    {
      QMutexLocker lock(&_mutex);
      _window.clear();
      _window_start = 0;
      _read_pos = 0;
    }
    QIODevice::seek(0);
  }

  void SeekableStream::drain() {
    if (_draining) {
      return; // Stream::readData emits fillChanged, which re-enters this slot
    }
    _draining = true;
    const qint64 available = _source.bytesAvailable();
    const QByteArray chunk = available > 0 ? _source.read(available) : QByteArray();
    _draining = false;

    if (chunk.isEmpty()) {
      return;
    }
    {
      QMutexLocker lock(&_mutex);
      _window.append(chunk);
      const qint64 excess = static_cast<qint64>(_window.size()) - _window_bytes;
      if (excess > 0) {
        _window.remove(0, static_cast<int>(excess));
        _window_start += excess;
      }
    }
    emit readyRead();
  }

  bool SeekableStream::isSequential() const {
    return false;
  }

  bool SeekableStream::atEnd() const {
    return false; // a live stream has no end; the default would report one whenever the window is drained
  }

  qint64 SeekableStream::pos() const {
    QMutexLocker lock(&_mutex);
    return _read_pos;
  }

  qint64 SeekableStream::size() const {
    QMutexLocker lock(&_mutex);
    return _window_start + _window.size();
  }

  qint64 SeekableStream::bytesAvailable() const {
    QMutexLocker lock(&_mutex);
    return qMax<qint64>(0, _window_start + _window.size() - _read_pos);
  }

  bool SeekableStream::seek(qint64 pos) {
    {
      QMutexLocker lock(&_mutex);
      if (pos < _window_start || pos > _window_start + _window.size()) {
        return false;
      }
      _read_pos = pos;
    }
    return QIODevice::seek(pos);
  }

  qint64 SeekableStream::readData(char *data, qint64 maxlen) {
    QMutexLocker lock(&_mutex);
    const qint64 end = _window_start + _window.size();
    // never -1: DirectShowIOReader zero-fills at buffer + bytesRead without
    // checking the sign, so an error return corrupts its sample buffer
    if (maxlen <= 0 || _read_pos < _window_start || _read_pos >= end) {
      return 0;
    }
    const qint64 bytes_read = qMin(maxlen, end - _read_pos);
    memcpy(data, _window.constData() + (_read_pos - _window_start), static_cast<size_t>(bytes_read));
    _read_pos += bytes_read;
    return bytes_read;
  }

  qint64 SeekableStream::writeData(const char *data, qint64 len) {
    Q_UNUSED(data)
    Q_UNUSED(len)
    return -1;
  }
}
