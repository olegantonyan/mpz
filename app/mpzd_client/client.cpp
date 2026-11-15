#include "client.h"

#include <QDebug>

#include <memory>

namespace MpzdClient {
  constexpr int RECONNECT_MS = 3000;
  constexpr int TRACKS_PAGE = 2000;

  Client::Client(QObject *parent) : QObject{parent} {
    reconnect_timer.setSingleShot(true);
    reconnect_timer.setInterval(RECONNECT_MS);
    connect(&reconnect_timer, &QTimer::timeout, this, &Client::connectSocket);
    connect(&socket, &QTcpSocket::readyRead, this, &Client::onReadyRead);
    connect(&socket, &QTcpSocket::disconnected, this, &Client::onSocketClosed);
    connect(&socket, &QAbstractSocket::errorOccurred, this, &Client::onSocketError);
  }

  QPair<bool, QString> Client::probe(const QUrl &url, int timeout_ms) {
    QTcpSocket s;
    s.connectToHost(url.host(), url.port(DEFAULT_PORT));
    if (!s.waitForConnected(timeout_ms)) {
      return {false, s.errorString()};
    }
    s.write(encode({"version"}));
    s.write(encode({"libraries"}));
    Parser p;
    QList<Item> items;
    int replies = 0;
    while (replies < 2) {
      if (!s.waitForReadyRead(timeout_ms)) {
        return {false, s.errorString()};
      }
      for (const auto &it : p.feed(s.readAll())) {
        if (it.kind == Item::Greeting && it.text != QString("mpzd %1").arg(PROTOCOL_VERSION)) {
          return {false, QObject::tr("not an mpzd server: %1").arg(it.text)};
        }
        if (it.kind == Item::Done) {
          items.append(it);
          replies++;
        }
      }
    }
    const QString version = items.at(0).reply.value("version");
    const int libraries = items.at(1).reply.records("library").size();
    return {true, QObject::tr("mpzd %1, %n library folder(s)", "", libraries).arg(version)};
  }

  QUrl Client::currentUrl() const {
    return url;
  }

  bool Client::isConnected() const {
    return ready;
  }

  const Status &Client::status() const {
    return last_status;
  }

  void Client::openConnection(const QUrl &new_url) {
    if (new_url == url && (ready || socket.state() != QAbstractSocket::UnconnectedState)) {
      return;
    }
    const bool was_ready = ready;
    const QUrl previous = url;
    url = new_url;
    error_reported = false;
    reconnect_timer.stop();
    ready = false;
    socket.abort();
    failPending(tr("connection reset"));
    if (was_ready) {
      emit disconnected(previous);
    }
    connectSocket();
  }

  void Client::closeConnection() {
    reconnect_timer.stop();
    const bool was_ready = ready;
    const QUrl previous = url;
    url = QUrl();
    ready = false;
    socket.abort();
    failPending(tr("connection closed"));
    last_status = Status();
    if (was_ready) {
      emit disconnected(previous);
    }
  }

  void Client::connectSocket() {
    if (url.isEmpty()) {
      return;
    }
    parser.reset();
    socket.abort();
    socket.connectToHost(url.host(), url.port(DEFAULT_PORT));
  }

  void Client::request(const QStringList &args, QObject *context, Callback callback) {
    if (socket.state() != QAbstractSocket::ConnectedState || !ready) {
      if (callback) {
        Reply r;
        r.code = -1;
        r.message = tr("not connected");
        callback(r);
      }
      return;
    }
    pending.enqueue({context, context != nullptr, std::move(callback)});
    socket.write(encode(args));
  }

  void Client::fetchTracks(quint32 playlist_id, const QString &playlist_name, QObject *context, std::function<void(bool, const QVector<Track> &)> callback) {
    auto acc = std::make_shared<QVector<Track>>();
    const QUrl server = url;
    const QString id = QString::number(playlist_id);
    request({"pl_tracks", id, "0", QString::number(TRACKS_PAGE)}, context, [=](const Reply &first) {
      if (!first.ok) {
        callback(false, QVector<Track>());
        return;
      }
      const int total = first.value("total").toInt();
      *acc = tracksFromReply(first, playlist_id, playlist_name, server);
      if (acc->size() >= total) {
        callback(true, *acc);
        return;
      }
      auto remaining = std::make_shared<int>(0);
      auto failed = std::make_shared<bool>(false);
      auto pages = std::make_shared<QMap<int, QVector<Track>>>();
      for (int from = TRACKS_PAGE; from < total; from += TRACKS_PAGE) {
        (*remaining)++;
        request({"pl_tracks", id, QString::number(from), QString::number(TRACKS_PAGE)}, context, [=](const Reply &page) {
          if (!page.ok) {
            *failed = true;
          } else {
            pages->insert(from, tracksFromReply(page, playlist_id, playlist_name, server));
          }
          if (--(*remaining) > 0) {
            return;
          }
          if (*failed) {
            callback(false, QVector<Track>());
            return;
          }
          for (const auto &chunk : std::as_const(*pages)) {
            acc->append(chunk);
          }
          callback(true, *acc);
        });
      }
    });
  }

  void Client::refreshStatus() {
    if (status_requested) {
      return;
    }
    status_requested = true;
    request({"status"}, this, [this](const Reply &reply) {
      status_requested = false;
      if (!reply.ok) {
        return;
      }
      last_status = Status::parse(reply);
      emit statusChanged(last_status);
    });
    if (!ready) {
      status_requested = false;
    }
  }

  void Client::failPending(const QString &message) {
    status_requested = false;
    Reply r;
    r.code = -1;
    r.message = message;
    while (!pending.isEmpty()) {
      auto p = pending.dequeue();
      if (p.callback && (!p.has_context || p.context)) {
        p.callback(r);
      }
    }
  }

  void Client::handleEvent(const QString &event) {
    const QStringList parts = event.split(' ');
    const QString &kind = parts.first();
    if (kind == "player") {
      refreshStatus();
    } else if (kind == "playlists") {
      emit playlistsChanged();
    } else if (kind == "playlist" && parts.size() > 1) {
      emit playlistChanged(parts.at(1).toUInt());
    } else if (kind == "scan" && parts.size() > 1) {
      emit scanDone(parts.at(1).toUInt());
    }
  }

  void Client::onReadyRead() {
    const auto items = parser.feed(socket.readAll());
    for (const auto &it : items) {
      if (it.kind == Item::Greeting) {
        if (it.text != QString("mpzd %1").arg(PROTOCOL_VERSION)) {
          const QString msg = tr("not an mpzd server: %1").arg(it.text);
          qWarning() << "mpzd:" << msg;
          socket.abort();
          if (!error_reported) {
            error_reported = true;
            emit error(url, msg);
          }
          return;
        }
        ready = true;
        error_reported = false;
        qDebug() << "connected to mpzd at" << url;
        request({"watch", "on"});
        emit connected(url);
        refreshStatus();
      } else if (it.kind == Item::Event) {
        handleEvent(it.text);
      } else if (pending.isEmpty()) {
        qWarning() << "mpzd: unexpected reply";
      } else {
        auto p = pending.dequeue();
        if (!it.reply.ok && !p.callback) {
          qWarning() << "mpzd error" << it.reply.code << it.reply.message;
        }
        if (p.callback && (!p.has_context || p.context)) {
          p.callback(it.reply);
        }
      }
    }
  }

  void Client::onSocketClosed() {
    if (url.isEmpty()) {
      return;
    }
    const bool was_ready = ready;
    ready = false;
    failPending(tr("connection lost"));
    if (was_ready) {
      qWarning() << "mpzd connection lost with" << url;
      emit error(url, tr("mpzd connection lost"));
      emit disconnected(url);
    }
    reconnect_timer.start();
  }

  void Client::onSocketError() {
    if (url.isEmpty() || socket.error() == QAbstractSocket::RemoteHostClosedError) {
      return;
    }
    if (!ready) {
      if (!error_reported) {
        error_reported = true;
        qWarning() << "mpzd:" << socket.errorString();
        emit error(url, socket.errorString());
      }
      reconnect_timer.start();
    }
  }
}
