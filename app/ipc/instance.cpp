#include "instance.h"

#include <QEventLoop>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QApplication>

namespace IPC {
  Instance::Instance(int timeo, int prt, QObject *parent) : QObject(parent), timeout_ms(timeo), port(prt) {
    connect(&server, &QTcpServer::newConnection, this, &Instance::on_server_connection);
  }

  bool Instance::isAnotherRunning() const {
    return send(QVariantMap());
  }

  bool Instance::send(const QVariantMap &data) const {
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeout_ms);

    auto bytes = QJsonDocument::fromVariant(data).toJson(QJsonDocument::Compact);

    QTcpSocket socket;
    socket.setSocketOption(QAbstractSocket::LowDelayOption, true);

    bool ok = true;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    auto conn_error = connect(&socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred), [&](QAbstractSocket::SocketError code) {
#else
    auto conn_error = connect(&sock, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), [&](QAbstractSocket::SocketError code) {
#endif
      Q_UNUSED(code);
      ok = false;
    });

    socket.connectToHost(QHostAddress::LocalHost, port);
    socket.waitForConnected(timeout_ms);
    if (socket.state() == QTcpSocket::ConnectedState) {
      socket.write("POST / HTTP/1.1\r\n\r\n");
      socket.write("Content-Type: application/json\r\n");
      socket.write(QString("Content-Length: %1\r\n").arg(bytes.length()).toUtf8());
      socket.write("Connection: Close\r\n");
      socket.write("\r\n");
      socket.write(bytes);
      socket.waitForBytesWritten(timeout_ms);
    } else {
      ok = false;
    }
    socket.close();
    disconnect(conn_error);

    return ok;
  }

  void Instance::start() {
    server.listen(QHostAddress::LocalHost, port);
  }

  QByteArray Instance::response() const {
    QByteArray result("HTTP/1.1 200 OK\r\n\r\n");
    result.append("Connection: close\r\n");
    result.append("\r\n");
    return result;
  }

  void Instance::process(const QByteArray &request) {
    auto string = QString::fromStdString(request.toStdString());
    auto content = string.split("\r\n").last();
    auto json = QJsonDocument::fromJson(content.toUtf8());
    if (json.isObject()) {
      qDebug() << json.toVariant().toMap();
      emit received(json.toVariant().toMap());
    }
  }

  QUrl Instance::url() const {
    auto i = QString("http://%1:%2").arg(QHostAddress(QHostAddress::LocalHost).toString()).arg(port);
    return QUrl(i);
  }

  void Instance::on_server_connection() {
    QTcpSocket *socket = server.nextPendingConnection();
    QTimer timer;
    QEventLoop loop;

    timer.setSingleShot(true);
    timer.setInterval(timeout_ms);
    timer.start();
    auto conn_read = connect(socket, &QTcpSocket::readyRead, [&]() {
      process(socket->readAll());
      socket->write(response());
      socket->waitForBytesWritten(timeout_ms);
      loop.quit();
    });
    auto conn_timer = connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    loop.exec();

    timer.stop();
    disconnect(conn_read);
    disconnect(conn_timer);

    socket->close();
  }
}
