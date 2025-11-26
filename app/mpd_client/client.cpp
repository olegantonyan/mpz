#include "client.h"

#include <QDebug>

namespace MpdClient {
  Client::Client(QObject *parent) : QObject{parent} {
    thread = new QThread;
    conn = new Connection();
    connect(conn, &Connection::connected, this, &Client::connected);
    connect(conn, &Connection::disconnected, this, &Client::disconnected);
    connect(conn, &Connection::error, this, &Client::error);
    connect(conn, &Connection::idleEvent, this, &Client::on_idleEvent);
    conn->moveToThread(thread);
    thread->start();
  }

  Client::~Client() {
    thread->quit();
    thread->wait();
    conn->deleteLater();
    thread->deleteLater();
  }

  QPair<bool, QString> Client::probe(const QUrl &url) {
    return conn->probe(url);
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

  void Client::play(const QString &playlist_name, int position) {
    QMetaObject::invokeMethod(
      conn,
      "play",
      Qt::QueuedConnection,
      Q_ARG(QString, playlist_name),
      Q_ARG(int, position)
    );
  }

  void Client::pause() {
    QMetaObject::invokeMethod(
      conn,
      "pause",
      Qt::QueuedConnection
    );
  }

  void Client::unpause() {
    QMetaObject::invokeMethod(
      conn,
      "unpause",
      Qt::QueuedConnection
    );
  }

  void Client::stop() {
    QMetaObject::invokeMethod(
      conn,
      "stop",
      Qt::QueuedConnection
    );
  }

  void Client::next() {
    QMetaObject::invokeMethod(
      conn,
      "next",
      Qt::QueuedConnection
    );
  }

  void Client::prev() {
    QMetaObject::invokeMethod(
      conn,
      "prev",
      Qt::QueuedConnection
    );
  }

  Song Client::currentSong() {
    Song result;
    QMetaObject::invokeMethod(
      conn,
      "currentSong",
      QThread::currentThread() == thread ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
      Q_RETURN_ARG(Song, result)
    );
    return result;
  }

  void Client::setVolume(int volume) {
    QMetaObject::invokeMethod(
      conn,
      "setVolume",
      Qt::QueuedConnection,
      Q_ARG(int, volume)
    );
  }

  void Client::setPosition(int pos) {
    QMetaObject::invokeMethod(
      conn,
      "setPosition",
      Qt::QueuedConnection,
      Q_ARG(int, pos)
    );
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
