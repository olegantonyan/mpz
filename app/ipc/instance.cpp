#include "instance.h"

#include <QEventLoop>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QApplication>

namespace IPC {
  static const auto STATUS_LINE_OK = "HTTP/1.1 257 UR U OK?\r\n\r\n";

  Instance::Instance(int prt, int timeo, QObject *parent) : QObject(parent), timeout_ms(timeo), port(prt) {
    connect(&server, &QTcpServer::newConnection, this, &Instance::on_server_connection);
  }

  int Instance::anotherPid() const {
    return send(QVariantMap());
  }

  int Instance::send(const QVariantMap &data) const {
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeout_ms);

    auto bytes = QJsonDocument::fromVariant(data).toJson(QJsonDocument::Compact);

    QTcpSocket socket;

    int pid = -1;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    auto conn_error = connect(&socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred), [&](QAbstractSocket::SocketError code) {
#else
    auto conn_error = connect(&sock, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), [&](QAbstractSocket::SocketError code) {
#endif
      Q_UNUSED(code);
      pid = -1;
    });

    socket.connectToHost(QHostAddress::LocalHost, port);
    socket.waitForConnected(timeout_ms);
    if (socket.state() == QTcpSocket::ConnectedState) {
      socket.write("POST / HTTP/1.1\r\n\r\n");
      socket.write("\r\n");
      socket.write(bytes);
      socket.waitForBytesWritten(timeout_ms);
      if (socket.waitForReadyRead(timeout_ms)) {
        auto data = QString::fromUtf8(socket.readAll());
        if (data.startsWith(STATUS_LINE_OK)) {
          auto body = QJsonDocument::fromJson(data.split("\r\n").last().toUtf8());
          if (body.isObject()) {
            pid = body["pid"].toInt();
          }
        }
      }
    }
    socket.close();
    disconnect(conn_error);

    return pid;
  }

  bool Instance::start() {
    if (!server.listen(QHostAddress::LocalHost, port)) {
      qWarning() << "error starting tcp server instance" << server.errorString();
      return false;
    }
    qDebug() << "first instance started, listen tcp port" << port << "pid" << qApp->applicationPid();
    return true;
  }

  bool Instance::load_files_send(const QStringList &list) {
    QVariantMap map;
    map["load_files"] = list;
    return send(map) > 0;
  }

  QByteArray Instance::process(const QByteArray &request) {
    QByteArray result(STATUS_LINE_OK);
    result.append("\r\n");

    auto string = QString::fromStdString(request.toStdString());
    auto content = string.split("\r\n").last();
    auto json = QJsonDocument::fromJson(content.toUtf8());
    if (json.isObject()) {
      if (json["load_files"].isArray()) {
        QStringList lst;
        for (auto i : json["load_files"].toArray()) {
          lst.append(i.toString());
        }
        emit load_files_received(lst);
      }
    }

    QVariantMap map;
    map["pid"] = qApp->applicationPid();
    result.append(QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact));

    return result;
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
      socket->write(process(socket->readAll()));
      socket->waitForBytesWritten(timeout_ms);
      socket->flush();
      socket->disconnectFromHost();
      loop.quit();
    });
    auto conn_timer = connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    loop.exec();

    timer.stop();
    disconnect(conn_read);
    disconnect(conn_timer);

    socket->close();
    socket->deleteLater();
  }
}
