#include "client.h"

#include <QDebug>

namespace MpdClient {
  Client::Client(QObject *parent) : QObject{parent} {
    thread = new QThread;
    conn = new Connection(thread);
    thread->start();
  }

  Client::~Client() {
    thread->quit();
    thread->wait();
    delete conn;
    delete thread;
  }

  void Client::establishConnection(const QUrl &url) {
    QMetaObject::invokeMethod(
      conn,
      "establish",
      Qt::QueuedConnection,
      Q_ARG(QUrl, url)
    );
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
