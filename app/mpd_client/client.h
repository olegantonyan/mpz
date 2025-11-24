#ifndef MPD_CLIENT_CLIENT_H
#define MPD_CLIENT_CLIENT_H

#include "mpd_client/connection.h"

#include <QObject>
#include <QThread>
#include <QVector>

namespace MpdClient {
  class Client : public QObject {
    Q_OBJECT
  public:
    explicit Client(QObject *parent = nullptr);
    ~Client();

    Connection *conn;

    void establishConnection(const QUrl &url);
    bool ping();
    QVector<Entity> lsDir(const QString &path);
    Status status();
    QVector<Song> lsPlaylistSongs(const QString &playlist_name);
    QVector<Song> lsDirsSongs(const QStringList &paths);
    bool appendSongsToPlaylist(const QStringList &paths, const QString &playlist_name);
    bool removeSongsFromPlaylist(const QVector<int> &indecies, const QString &playlist_name);

  signals:

  private:
    QThread *thread;
  };
}

#endif // MPD_CLIENT_CLIENT_H
