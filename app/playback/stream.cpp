#include "stream.h"

#include <QDebug>
#include <QtConcurrent>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Playback {
  Stream::Stream(quint32 threshold_bytes, QObject *parent) : QIODevice(parent), _running(false), _threshold_bytes(threshold_bytes) {
    open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    connect(this, &Stream::started, [=]() {
      _running = true;
      qDebug() << "stream started";
    });
    connect(this, &Stream::stopped, [=]() {
      _running = false;
      setUrl(QUrl());
      qDebug() << "stream stopped";
    });
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

    QtConcurrent::run(this, &Stream::thread);

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
    return _running;
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

      QEventLoop loop;
      auto conn = connect(this, &Stream::stopped, &loop, &QEventLoop::quit);
      loop.exec();
      disconnect(conn);
    }
  }

  void Stream::append(const QByteArray &a) {
    if (a.isEmpty()) {
      return;
    }
    _mutex.lock();
    _buffer.append(a);
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

    QEventLoop loop;
    QNetworkAccessManager nam;
    QNetworkRequest request(url());
    QNetworkReply *reply = nam.get(request);

    auto conn_read = connect(reply, &QNetworkReply::downloadProgress, [=](qint64 bytesReceived, qint64 bytesTotal) {
      Q_UNUSED(bytesReceived)
      Q_UNUSED(bytesTotal)
      append(reply->readAll());
    });

    auto conn_abort = connect(this, &Stream::stopping, reply, &QNetworkReply::abort);
    auto conn_quit = connect(this, &Stream::stopping, &loop, &QEventLoop::quit);
    loop.exec();
    disconnect(conn_read);
    disconnect(conn_abort);
    disconnect(conn_quit);
    reply->deleteLater();
    clear();

    emit stopped();
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
