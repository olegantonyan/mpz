#include "instance.h"

#include <memory>

#include <QEventLoop>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QApplication>
#include <QLocalSocket>
#include <QAbstractSocket>
#include <QStandardPaths>
#include <QDir>

namespace IPC {
  static const auto STATUS_LINE_RESPONSE_OK = "HTTP/1.1 257 R U OK\r\n\r\n";
  static const auto STATUS_LINE_REQUEST = "POST / HTTP/1.1\r\n\r\n";
  static const qint64 MAX_PAYLOAD_BYTES = 64 * 1024;

  Instance::Instance(int timeo, QObject *parent) : QObject(parent), timeout_ms(timeo), socket_name(socketName()) {
    connect(&server, &QLocalServer::newConnection, this, &Instance::on_server_connection);
  }

  QString Instance::socketName() {
#ifdef Q_OS_WIN
    return QStringLiteral("org.mpz_player.mpz");
#else
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/single_instance");
#endif
  }

  int Instance::anotherPid() const {
    return send(QVariantMap());
  }

  int Instance::send(const QVariantMap &data) const {
    QLocalSocket socket;

    int pid = -1;
    socket.connectToServer(socket_name);
    if (socket.waitForConnected(timeout_ms)) {
      socket.write(STATUS_LINE_REQUEST);
      socket.write("\r\n");
      socket.write(QJsonDocument::fromVariant(data).toJson(QJsonDocument::Compact));
      socket.waitForBytesWritten(timeout_ms);
      if (socket.waitForReadyRead(timeout_ms)) {
        auto recvd_data = QString::fromUtf8(socket.readAll());
        if (recvd_data.startsWith(STATUS_LINE_RESPONSE_OK)) {
          auto body = QJsonDocument::fromJson(recvd_data.split("\r\n").last().toUtf8());
          if (body.isObject()) {
            pid = body.object()["pid"].toInt();
          }
        }
      }
    }
    socket.disconnectFromServer();

    return pid;
  }

  bool Instance::start() {
    server.setSocketOptions(QLocalServer::UserAccessOption);
    if (server.listen(socket_name)) {
      qDebug() << "first instance started, listening on" << server.fullServerName() << "pid" << qApp->applicationPid();
      return true;
    }
    if (server.serverError() == QAbstractSocket::AddressInUseError) {
      if (anotherPid() > 0) {
        return false;
      }
      QLocalServer::removeServer(socket_name);
      if (server.listen(socket_name)) {
        qDebug() << "first instance started after clearing stale socket, listening on" << server.fullServerName();
        return true;
      }
    }
    qWarning() << "error starting local server instance" << server.errorString();
    return false;
  }

  bool Instance::load_files_send(const QStringList &list) {
    QVariantMap map;
    map["load_files"] = list;
    return send(map) > 0;
  }

  QByteArray Instance::process_received(const QByteArray &request) {
    QByteArray response(STATUS_LINE_RESPONSE_OK);
    response.append("\r\n");

    auto body = QString::fromStdString(request.toStdString()).split("\r\n").last();
    auto json_body = QJsonDocument::fromJson(body.toUtf8());
    if (json_body.isObject()) {
      if (json_body.object()["load_files"].isArray()) {
        QStringList lst;
        for (const auto &i : json_body.object()["load_files"].toArray()) {
          lst.append(i.toString());
        }
        emit load_files_received(lst);
      }
    }

    QVariantMap json_body_response;
    json_body_response["pid"] = qApp->applicationPid();
    response.append(QJsonDocument::fromVariant(json_body_response).toJson(QJsonDocument::Compact));

    return response;
  }

  void Instance::on_server_connection() {
    QLocalSocket *socket = server.nextPendingConnection();
    if (!socket) {
      return;
    }
    QTimer timer;
    QEventLoop loop;
    auto buffer = std::make_shared<QByteArray>();

    timer.setSingleShot(true);
    timer.setInterval(timeout_ms);
    timer.start();
    auto conn_read = connect(socket, &QLocalSocket::readyRead, this, [&, buffer]() {
      buffer->append(socket->readAll());
      if (buffer->size() > MAX_PAYLOAD_BYTES) {
        qWarning() << "ipc payload exceeds" << MAX_PAYLOAD_BYTES << "bytes, dropping";
        socket->abort();
        loop.quit();
        return;
      }
      if (!buffer->contains("\r\n\r\n")) {
        return;
      }
      socket->write(process_received(*buffer));
      socket->waitForBytesWritten(timeout_ms);
      socket->flush();
      socket->disconnectFromServer();
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
