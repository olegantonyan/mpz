#ifndef MPD_CLIENT_CONNECTION_H
#define MPD_CLIENT_CONNECTION_H

#include "entity.h"
#include "status.h"
#include "song.h"

#include <QObject>
#include <QSocketNotifier>
#include <QTimer>
#include <QPair>
#include <QUrl>
#include <QThread>
#include <QVector>

#include "mpd/client.h"

namespace MpdClient {
  class Connection : public QObject {
    Q_OBJECT
  public:
    explicit Connection(QThread *thread);
    ~Connection();

  public slots:
    bool open(const QUrl &url);
    void close();
    QPair<bool, QString> probe(const QUrl &url);
    bool ping();
    QUrl currentUrl() const;

    QVector<Entity> lsDir(const QString &path);
    Status status();
    QVector<Song> lsPlaylistSongs(const QString &playlist_name);
    QVector<Song> lsDirsSongs(const QStringList &paths);
    bool appendSongsToPlaylist(const QStringList &paths, const QString &playlist_name);
    bool removeSongsFromPlaylist(const QVector<int> &indecies, const QString &playlist_name);
    QVector<Entity> playlists();
    bool removePlaylist(const QString &playlist_name);
    bool createPlaylist(const QStringList &song_paths, const QString &playlist_name);
    bool play(const QString &playlist_name, int position);
    bool pause();
    bool unpause();
    bool stop();
    bool next();
    bool prev();
    Song currentSong();

  signals:
    void connected(const QUrl &url);
    void disconnected(const QUrl &url);
    void error(const QUrl &url);
    void idleEvent(mpd_idle event);

  private slots:
    void on_idle_readable();

  private:
    struct mpd_connection *conn;
    struct mpd_connection *idle_conn;
    QSocketNotifier *idle_notifier;
    QUrl current_connection_url;
    QTimer conn_timer;

    bool establish_idle(const QUrl &url);
    void destroy();
    void waitConnected();
    QString lastError() const;
  };
}

#endif // MPD_CLIENT_CONNECTION_H
