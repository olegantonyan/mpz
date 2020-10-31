#include "instance.h"

#include <QEventLoop>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QApplication>

namespace IPC {
  Instance::Instance(int prt, int timeo, QObject *parent) : QObject(parent), timeout_ms(timeo), port(prt) {
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
      if (socket.waitForReadyRead(timeout_ms)) {
        auto data = QString::fromUtf8(socket.readAll());
        if (data.startsWith("HTTP/1.1 200 OK\r\n\r\n")) {
          ok = true;
        } else {
          ok = false;
        }
      } else {
        ok = false;
      }
    } else {
      ok = false;
    }
    socket.close();
    disconnect(conn_error);

    return ok;
  }

  bool Instance::start() {
    if (!server.listen(QHostAddress::LocalHost, port)) {
      qWarning() << "error starting tcp server instance" << server.errorString();
      return false;
    }
    qDebug() << "first instance started";
    return true;
  }

  bool Instance::load_files_send(const QStringList &list) {
    qDebug() << "sending arguments to another running instance";
    QVariantMap map;
    map["load_files"] = list;
    return send(map);
  }

  QByteArray Instance::process(const QByteArray &request) {
    QByteArray result("HTTP/1.1 200 OK\r\n\r\n");
    result.append("Connection: close\r\n");
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
