#include "client.h"

#include <QDebug>

namespace MpdClient {
  Client::Client(QObject *parent) : QObject{parent} {
    thread = new QThread;
    conn = new Connection(thread);
    connect(conn, &Connection::connected, this, &Client::connected);
    connect(conn, &Connection::disconnected, this, &Client::disconnected);
    connect(conn, &Connection::error, this, &Client::error);
    connect(conn, &Connection::idleEvent, this, &Client::on_idleEvent);
    thread->start();
  }

  Client::~Client() {
    thread->quit();
    thread->wait();
    delete conn;
    delete thread;
  }

  void Client::openConnection(const QUrl &url) {
    QMetaObject::invokeMethod(
      conn,
      "open",
      Qt::QueuedConnection,
      Q_ARG(QUrl, url)
    );
  }

  void Client::closeConnection() {
    QMetaObject::invokeMethod(
      conn,
      "close",
      Qt::QueuedConnection
    );
  }

  QUrl Client::currentUrl() const {
    return conn->currentUrl();
  }

  bool Client::ping() {
    bool result = false;
    QMetaObject::invokeMethod(
      conn,
      "ping",
      QThread::currentThread() == thread ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
      Q_RETURN_ARG(bool, result)
    );
    return result;
  }

  Status Client::status() {
    MpdClient::Status result;

    QMetaObject::invokeMethod(
      conn,
      "status",
      QThread::currentThread() == thread ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
      Q_RETURN_ARG(MpdClient::Status , result)
    );
    return result;
  }

  QVector<Song> Client::lsPlaylistSongs(const QString &playlist_name) {
    QVector<MpdClient::Song> result;

    QMetaObject::invokeMethod(
      conn,
      "lsPlaylistSongs",
      QThread::currentThread() == thread ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
      Q_RETURN_ARG(QVector<MpdClient::Song>, result),
      Q_ARG(QString, playlist_name)
    );
    return result;
  }

  QVector<Song> Client::lsDirsSongs(const QStringList &paths) {
    QVector<MpdClient::Song> result;

    QMetaObject::invokeMethod(
      conn,
      "lsDirsSongs",
      QThread::currentThread() == thread ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
      Q_RETURN_ARG(QVector<MpdClient::Song>, result),
      Q_ARG(QStringList, paths)
    );
    return result;
  }

  bool Client::appendSongsToPlaylist(const QStringList &paths, const QString &playlist_name) {
    bool result = false;
    QMetaObject::invokeMethod(
      conn,
      "appendSongsToPlaylist",
      QThread::currentThread() == thread ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
      Q_RETURN_ARG(bool, result),
      Q_ARG(QStringList, paths),
      Q_ARG(QString, playlist_name)
    );
    return result;
  }

  bool Client::removeSongsFromPlaylist(const QVector<int> &indecies, const QString &playlist_name) {
    bool result = false;
    QMetaObject::invokeMethod(
      conn,
      "removeSongsFromPlaylist",
      QThread::currentThread() == thread ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
      Q_RETURN_ARG(bool, result),
      Q_ARG(QVector<int>, indecies),
      Q_ARG(QString, playlist_name)
    );
    return result;
  }

  QVector<Entity> Client::playlists() {
    QVector<MpdClient::Entity> result;

    QMetaObject::invokeMethod(
      conn,
      "playlists",
      QThread::currentThread() == thread ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
      Q_RETURN_ARG(QVector<MpdClient::Entity>, result)
    );
    return result;
  }

  bool Client::removePlaylist(const QString &playlist_name) {
    bool result = false;
    QMetaObject::invokeMethod(
      conn,
      "removePlaylist",
      QThread::currentThread() == thread ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
      Q_RETURN_ARG(bool, result),
      Q_ARG(QString, playlist_name)
    );
    return result;
  }

  bool Client::createPlaylist(const QStringList &paths, const QString &playlist_name) {
    bool result = false;
    QMetaObject::invokeMethod(
      conn,
      "createPlaylist",
      QThread::currentThread() == thread ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
      Q_RETURN_ARG(bool, result),
      Q_ARG(QStringList, paths),
      Q_ARG(QString, playlist_name)
    );
    return result;
  }

  bool Client::play(const QString &playlist_name, int position) {
    bool result = false;
    QMetaObject::invokeMethod(
      conn,
      "play",
      QThread::currentThread() == thread ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
      Q_RETURN_ARG(bool, result),
      Q_ARG(QString, playlist_name),
      Q_ARG(int, position)
    );
    return result;
  }

  bool Client::pause() {
    bool result = false;
    QMetaObject::invokeMethod(
      conn,
      "pause",
      QThread::currentThread() == thread ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
      Q_RETURN_ARG(bool, result)
    );
    return result;
  }

  bool Client::unpause() {
    bool result = false;
    QMetaObject::invokeMethod(
      conn,
      "unpause",
      QThread::currentThread() == thread ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
      Q_RETURN_ARG(bool, result)
    );
    return result;
  }

  bool Client::stop() {
    bool result = false;
    QMetaObject::invokeMethod(
      conn,
      "stop",
      QThread::currentThread() == thread ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
      Q_RETURN_ARG(bool, result)
    );
    return result;
  }

  bool Client::next() {
    bool result = false;
    QMetaObject::invokeMethod(
      conn,
      "next",
      QThread::currentThread() == thread ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
      Q_RETURN_ARG(bool, result)
    );
    return result;
  }

  bool Client::prev() {
    bool result = false;
    QMetaObject::invokeMethod(
      conn,
      "prev",
      QThread::currentThread() == thread ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
      Q_RETURN_ARG(bool, result)
    );
    return result;
  }

  void Client::on_idleEvent(mpd_idle event) {
    if (event & MPD_IDLE_DATABASE) {
      emit databaseUpdated();
    }
    if (event & MPD_IDLE_PLAYER) {
      emit playerStateChanged();
    }
    if (event & MPD_IDLE_STORED_PLAYLIST) {
      emit playlistUpdated();
    }
    if (event & MPD_IDLE_MIXER) {
      emit mixerChanged();
    }
  }

  QVector<MpdClient::Entity> MpdClient::Client::lsDir(const QString &path) {
    QVector<MpdClient::Entity> result;

    QMetaObject::invokeMethod(
      conn,
      "lsDir",
      QThread::currentThread() == thread ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
      Q_RETURN_ARG(QVector<MpdClient::Entity>, result),
      Q_ARG(QString, path)
    );
    return result;
  }
}
