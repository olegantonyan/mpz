#include "instance.h"

#include <QEventLoop>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QApplication>

namespace IPC {
  Instance::Instance(int timeo, int prt, QObject *parent) : QObject(parent), timeout_ms(timeo), port(prt) {
    connect(&server, &QTcpServer::newConnection, this, &Instance::on_server_connection);
  }

  bool Instance::isAnotherRunning() const {
    return send(QVariant());
  }

  bool Instance::send(const QVariant &data) const {
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeout_ms);

    QNetworkRequest request(url());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkAccessManager nam;
    QNetworkReply *reply = nam.post(request, QJsonDocument::fromVariant(data).toJson());
    connect(&timer, &QTimer::timeout, reply, &QNetworkReply::abort);
    while (!reply->isFinished()) {
      qApp->processEvents();
    }
    timer.stop();

    bool ok = reply->error() == QNetworkReply::NoError && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200;

    reply->deleteLater();
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
    emit received(json.toVariant());
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
