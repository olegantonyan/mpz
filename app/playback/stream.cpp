#include "stream.h"

#include <QDebug>
#include <QtConcurrent>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Playback {
  Stream::Stream(quint32 threshold_bytes, QObject *parent) :
    QIODevice(parent),
    _total_bytes_received(0),
    _threshold_bytes(threshold_bytes),
    _max_bytes(threshold_bytes * 128) {
    open(QIODevice::ReadOnly | QIODevice::Unbuffered);
  }

  Stream::~Stream() {
    stop();
  }

  bool Stream::start(quint32 timeout_ms) {
    if (!isValidUrl()) {
      return false;
    }
    qDebug() << "starting stream from" << url();

    _total_bytes_received = 0;
    clear();
    if (isRunning()) {
      stop();
    }

    _future = QtConcurrent::run(this, &Stream::thread);

    return waitForFill(timeout_ms);
  }

  bool Stream::waitForFill(quint32 timeout_ms) {
    if (bytesAvailable() >= _threshold_bytes) {
      return true;
    }

    bool ok = false;

    QEventLoop loop;
    auto ready_conn = connect(this, &Stream::readyRead, [&]() {
      if (bytesAvailable() >= _threshold_bytes) {
        loop.quit();
        ok = true;
      }
    });
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(static_cast<int>(timeout_ms));
    auto timer_conn = connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start();

    loop.exec();

    disconnect(ready_conn);
    disconnect(timer_conn);
    return ok;
  }

  bool Stream::isRunning() const {
    return _future.isRunning();
  }

  bool Stream::isValidUrl() const {
    return url().scheme() == "http" || url().scheme() == "https";
  }

  void Stream::setUrl(const QUrl &url) {
    _url = url;
  }

  QUrl Stream::url() const {
    return _url;
  }

  void Stream::stop() {
    if (isRunning()) {
      emit stopping();
      _future.waitForFinished();
    }
  }

  void Stream::append(const QByteArray &a) {
    if (a.isEmpty()) {
      return;
    }
    _mutex.lock();
    if (static_cast<quint32>(_buffer.size()) < _max_bytes) {
      _buffer.append(a);
    }
    _total_bytes_received += a.size();
    quint32 new_size = static_cast<quint32>(_buffer.size());
    _mutex.unlock();
    if (_total_bytes_received >= _threshold_bytes) {
      emit readyRead();
    }
    emit fillChanged(new_size, _threshold_bytes);
  }

  void Stream::clear() {
    _mutex.lock();
    _buffer.clear();
    _mutex.unlock();
  }

  void Stream::thread() {
    emit started();

    qDebug() << "stream started";

    QEventLoop loop;
    QNetworkAccessManager nam;
    QNetworkRequest request(url());
    //request.setRawHeader("Icy-MetaData", "1");
    QNetworkReply *reply = nam.get(request);

    auto conn_read = connect(reply, &QNetworkReply::downloadProgress, [=](qint64 bytesReceived, qint64 bytesTotal) {
      Q_UNUSED(bytesReceived)
      Q_UNUSED(bytesTotal)
      //qDebug() << "icy-metaint" << reply->rawHeader("icy-metaint");
      append(reply->readAll());
    });
    auto conn_error = connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), [=](QNetworkReply::NetworkError code) {
      if (code != QNetworkReply::OperationCanceledError) {
        qWarning() << "stream network error" << code << reply->errorString();
        emit error(reply->errorString());
      }
    });

    auto conn_fin = connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    auto conn_abort = connect(this, &Stream::stopping, reply, &QNetworkReply::abort);
    auto conn_quit = connect(this, &Stream::stopping, &loop, &QEventLoop::quit);
    loop.exec();
    disconnect(conn_read);
    disconnect(conn_abort);
    disconnect(conn_quit);
    disconnect(conn_fin);
    disconnect(conn_error);
    reply->deleteLater();
    clear();

    setUrl(QUrl());

    emit stopped();

    qDebug() << "stream stopped";
  }

  bool Stream::isSequential() const {
    return true;
  }

  qint64 Stream::readData(char *data, qint64 maxlen) {
    _mutex.lock();
    int bytes_read = qMin(static_cast<int>(maxlen), static_cast<int>(_buffer.size()));
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
    return _total_bytes_received;
  }

  qint64 Stream::bytesAvailable() const {
    return _buffer.size();
  }
}
