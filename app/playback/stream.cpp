#include "stream.h"

#include <QDebug>
#include <QtConcurrent>
#include <QTimer>
#include <QTcpSocket>
#include <QMutexLocker>

namespace Playback {
  Stream::Stream(quint32 threshold_bytes, quint16 threshold_multiplier, QObject *parent) :
    QIODevice(parent),
    _total_bytes_received(0),
    _icy_metaint(0),
    _next_meta_pos(0),
    _threshold_bytes(threshold_bytes),
    _max_bytes(threshold_bytes * threshold_multiplier),
    _chunked(false),
    _chunk_state(ChunkState::Size),
    _chunk_remaining(0) {
    open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    _timeout_ms = 30000;
  }

  Stream::~Stream() {
    stop();
  }

  bool Stream::start() {
    if (!isValidUrl()) {
      emit error(QString("invalid url: %1").arg(url().toString()));
      return false;
    }
    qDebug() << "starting stream from" << url();

    if (isRunning()) {
      stop();
    }
    clear();

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    _future = QtConcurrent::run(&Stream::thread, this);
#else
    _future = QtConcurrent::run(this, &Stream::thread);
#endif

    return _future.isStarted();
  }

  bool Stream::isRunning() const {
    return _future.isRunning();
  }

  bool Stream::isValidUrl() const {
    return url().scheme() == "http" || url().scheme() == "https";
  }

  void Stream::setUrl(const QUrl &url) {
    QMutexLocker lock(&_mutex);
    _url = url;
    if (!_url.isEmpty() && _url.port() < 0) {
      if (_url.scheme() == "https") {
        _url.setPort(443);
      } else {
        _url.setPort(80);
      }

    }
  }

  QUrl Stream::url() const {
    QMutexLocker lock(&_mutex);
    return _url;
  }

  void Stream::stop() {
    if (isRunning()) {
      emit stopping();
      _future.waitForFinished();
    }
  }

  int Stream::append_extract_meta(const QByteArray &a) {
    if (_icy_metaint <= 0) {
      return -1;
    }
    int bytes_appended = 0;

    int meta_bytes = 0;
    QByteArray meta;
    for (int i = 0; i < a.size(); i++) {
      if (i + _total_bytes_received == _next_meta_pos) {
        meta_bytes = a.at(i) * 16;
        _next_meta_pos += (_icy_metaint + meta_bytes + 1);
      } else if (meta_bytes > 0) {
        meta.append(a.at(i));
        meta_bytes--;
        if (meta_bytes == 0) {
          _meta.insert("stream", meta);
          emit metadataChanged(_meta);
        }
      } else {
        bytes_appended++;
        _buffer.append(a.at(i));
      }
    }
    return bytes_appended;
  }

  QString Stream::buildRequest() const {
    QStringList headers;
    // HTTP/1.0: a 1.1 response to an infinite (no Content-Length) stream is framed
    // with Transfer-Encoding: chunked by proxies like Caddy, which the ffmpeg
    // backend can't parse. 1.0 forces a raw byte stream instead.
    headers.append(QString("GET %1 HTTP/1.0").arg(url().path()));
    headers.append(QString("Host: %1:%2").arg(url().host()).arg(url().port()));
    headers.append(QString("User-Agent: %1 %2").arg(qAppName()).arg(qApp->applicationVersion()));
    if (!url().userName().isEmpty() && !url().password().isEmpty()) {
      QString concatenated = url().userName() + ":" + url().password();
      QByteArray data = concatenated.toLocal8Bit().toBase64();
      QString header_data = "Basic " + data;
      headers.append(QString("Authorization: %1").arg(header_data));
    }
    headers.append("Icy-Metadata: 1");
    headers.append("Accept: */*");

    return headers.join("\r\n") + "\r\n\r\n";
  }

  QMap<QString, QString> Stream::parseHeaders(QStringList rawheaders) {
    QMap<QString, QString> result;
    rawheaders.removeAt(0); // status line. don't care
    rawheaders.removeAll({}); // empty and null
    for (const auto &i : std::as_const(rawheaders)) {
      // HTTP header names are case-insensitive; proxies (Caddy/Go) Title-Case them
      // (Icy-Metaint, Icy-Br, ...). Normalise to lower case so downstream lookups
      // here and in StreamMetaData (which reads "icy-br", "content-type", ...) match.
      auto key = i.section(':', 0, 0).trimmed().toLower();
      auto value = i.section(':', 1).trimmed();
      result.insert(key, value);
      if (key.startsWith("icy-") || key == "content-type") {
        _meta.insert(key, value);
        emit metadataChanged(_meta);
      }
    }
    return result;
  }

  void Stream::feed(const QByteArray& raw) {
    append(_chunked ? dechunk(raw) : raw);
  }

  QByteArray Stream::dechunk(const QByteArray& in) {
    QByteArray out;
    int i = 0;
    const int n = in.size();
    while (i < n && _chunk_state != ChunkState::Done) {
      switch (_chunk_state) {
      case ChunkState::Size: {
        const char c = in.at(i++);
        _chunk_size_line.append(c);
        if (c == '\n') {
          QByteArray line = _chunk_size_line.trimmed();
          const int semi = line.indexOf(';'); // drop chunk extensions
          if (semi >= 0) {
            line = line.left(semi);
          }
          bool ok = false;
          const qint64 sz = line.toLongLong(&ok, 16);
          _chunk_size_line.clear();
          if (!ok) {
            qWarning() << "stream: malformed chunk size" << line;
            _chunk_state = ChunkState::Done;
          } else if (sz == 0) {
            _chunk_state = ChunkState::Done; // terminating chunk; trailers ignored
          } else {
            _chunk_remaining = sz;
            _chunk_state = ChunkState::Data;
          }
        }
        break;
      }
      case ChunkState::Data: {
        const qint64 take = qMin<qint64>(_chunk_remaining, n - i);
        out.append(in.constData() + i, static_cast<int>(take));
        i += static_cast<int>(take);
        _chunk_remaining -= take;
        if (_chunk_remaining == 0) {
          _chunk_state = ChunkState::AfterData;
        }
        break;
      }
      case ChunkState::AfterData: {
        // skip the CRLF that terminates the chunk data
        if (in.at(i++) == '\n') {
          _chunk_state = ChunkState::Size;
        }
        break;
      }
      case ChunkState::Done:
        break;
      }
    }
    return out;
  }

  void Stream::append(const QByteArray& a) {
    if (a.isEmpty()) {
      return;
    }
    _mutex.lock();

    int bytes_appended = 0;
    if (_icy_metaint > 0) {
      bytes_appended = append_extract_meta(a);
    } else {
      _buffer.append(a);
      bytes_appended = a.size();
    }
    if (static_cast<quint32>(_buffer.size()) >= _max_bytes) {
      qDebug() << "stream buffer overflow";
      _buffer.remove(0, bytes_appended);
    }
    _total_bytes_received += a.size();

    quint32 new_size = static_cast<quint32>(_buffer.size());
    _mutex.unlock();
    if (new_size >= _threshold_bytes) {
      emit readyRead();
    }
    emit fillChanged(new_size, _threshold_bytes);
  }

  void Stream::clear() {
    _mutex.lock();
    _buffer.clear();
    _total_bytes_received = 0;
    _icy_metaint = 0;
    _next_meta_pos = 0;
    _meta.clear();
    _chunked = false;
    _chunk_state = ChunkState::Size;
    _chunk_remaining = 0;
    _chunk_size_line.clear();
    _mutex.unlock();
  }

  void Stream::thread() {
    emit started();

    qDebug() << "stream started";

    QEventLoop loop;
    QTcpSocket sock;
    QMap<QString, QString> headers;
    QByteArray header_buf;
    QTimer timer;

    sock.setSocketOption(QAbstractSocket::KeepAliveOption, 1);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    auto conn_error = connect(&sock, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred), &sock, [&](QAbstractSocket::SocketError code) {
#else
    auto conn_error = connect(&sock, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), &sock, [&](QAbstractSocket::SocketError code) {
#endif
      qWarning() << "stream network error" << code << sock.errorString();
      emit error(sock.errorString());
    });

    auto conn_read = connect(&sock, &QTcpSocket::readyRead, &sock, [&]() {
      auto data = sock.readAll();
      if (!headers.isEmpty()) {
        feed(data);
      } else {
        // HTTP headers may arrive split across multiple readyRead calls.
        // Accumulate into header_buf until we see the \r\n\r\n terminator.
        header_buf.append(data);
        const QByteArray HEADERS_TERMINATOR("\r\n\r\n");
        int term_idx = header_buf.indexOf(HEADERS_TERMINATOR);
        if (term_idx >= 0) {
          int body_idx = term_idx + HEADERS_TERMINATOR.size();
          headers = parseHeaders(QString::fromLatin1(header_buf.left(body_idx)).split("\r\n"));
          extract_icy_metaint(headers);
          _chunked = headers.value("transfer-encoding").contains("chunked", Qt::CaseInsensitive);
          QByteArray body = header_buf.mid(body_idx);
          header_buf.clear();
          feed(body);
        }
      }
      timer.stop();
      timer.setInterval(_timeout_ms);
      timer.start();
    });

    auto conn_fin = connect(&sock, &QTcpSocket::disconnected, &loop, &QEventLoop::quit);
    auto conn_abort = connect(this, &Stream::stopping, &sock, &QTcpSocket::disconnectFromHost);
    auto conn_quit = connect(this, &Stream::stopping, &loop, &QEventLoop::quit);

    timer.setSingleShot(true);
    timer.setInterval(_timeout_ms);
    timer.start();
    connect(&timer, &QTimer::timeout, &timer, [&]() {
      emit stopping();
      qWarning() << "stream timeout";
      emit error("timeout");
    });

    sock.connectToHost(url().host(), static_cast<quint16>(url().port(80)));
    sock.waitForConnected(5000); // cap the GUI-blocking window when tearing down mid-connect against a dead host
    sock.write(buildRequest().toLatin1());

    loop.exec();
    disconnect(conn_read);
    disconnect(conn_abort);
    disconnect(conn_quit);
    disconnect(conn_fin);
    disconnect(conn_error);
    sock.close();
    clear();
    {
      QMutexLocker lock(&_mutex);
      _url.clear();
    }

    emit stopped();

    qDebug() << "stream stopped";
  }

  bool Stream::extract_icy_metaint(const QMap<QString, QString> &headers) {
    const auto HEADER = "icy-metaint";

    if (_icy_metaint > 0) {
      return false;
    }
    if (!headers.contains(HEADER)) {
      return false;
    }
    auto raw_metaint = headers.value(HEADER);
    if (raw_metaint.isEmpty()) {
      return false;
    }
    bool ok = false;
    int raw_int = raw_metaint.toInt(&ok);
    if (!ok) {
      return false;
    }
    _icy_metaint = raw_int;
    _next_meta_pos = _icy_metaint;
    return true;
  }

  bool Stream::isSequential() const {
    return true;
  }

  qint64 Stream::readData(char *data, qint64 maxlen) {
    _mutex.lock();
    qint64 bytes_read = qMin(maxlen, static_cast<qint64>(_buffer.size()));
    memcpy(data, _buffer.data(), static_cast<size_t>(bytes_read));
    _buffer.remove(0, bytes_read);
    quint32 new_size = static_cast<quint32>(_buffer.size());
    _mutex.unlock();
    emit fillChanged(new_size, _threshold_bytes);
    return bytes_read;
  }

  qint64 Stream::writeData(const char *data, qint64 len) {
    // not supposed to be implemented
    Q_UNUSED(data)
    Q_UNUSED(len)
    return -1;
  }

  qint64 Stream::pos() const {
    QMutexLocker locker(&_mutex);
    return _total_bytes_received;
  }

  qint64 Stream::bytesAvailable() const {
    QMutexLocker locker(&_mutex);
    return _buffer.size();
  }
}
