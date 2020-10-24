#include "instance.h"

#include <QEventLoop>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace IPC {
  Instance::Instance(QObject *parent) : QObject(parent) {
    server.listen(QHostAddress::LocalHost, 10001);
    connect(&server, &QTcpServer::newConnection, this, &Instance::on_server_connection);
  }

  bool Instance::isAnotherRunning() const {
    return false;
  }

  void Instance::send(const QVariant &data) {

  }

  QByteArray Instance::response(bool ok) const {
    QByteArray result("HTTP/1.1 ");
    if (ok) {
      result.append("200 OK");
    } else {
      result.append("400 Bad Request");
    }
    result.append("\r\n\r\n");

    QStringList headers;
    //headers << "Content-Type: application/json; charset=UTF-8";
    //headers << "Content-Length: 0";
    headers << "Connection: close";

    result.append(headers.join("\r\n").toUtf8());
    result.append("\r\n");

    return result;
  }

  bool Instance::process(const QByteArray &request) {
    auto string = QString::fromStdString(request.toStdString());
    auto content = string.split("\r\n").last();
    auto json = QJsonDocument::fromJson(content.toUtf8());
    if (!json["files"].isArray()) {
      return false;
    }
    qDebug() << json["files"].toArray();

    return true;
  }

  void Instance::on_server_connection() {
    QTcpSocket *socket = server.nextPendingConnection();
    QTimer timer;
    QEventLoop loop;

    timer.setSingleShot(true);
    timer.setInterval(5000);
    auto conn_read = connect(socket, &QTcpSocket::readyRead, [&]() {
      bool ok = process(socket->readAll());
      socket->write(response(ok));
      socket->waitForBytesWritten(5000);
      loop.quit();
    });
    auto conn_timer = connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    loop.exec();
    disconnect(conn_read);
    disconnect(conn_timer);

    socket->close();
  }
}
