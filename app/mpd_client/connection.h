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
    bool establish(const QUrl &url);
    QPair<bool, QString> probe(const QUrl &url);
    bool ping();
    void destroy();
    QUrl currentUrl() const;

    QVector<Entity> lsDir(const QString &path);
    Status status();
    QVector<Song> lsPlaylistSongs(const QString &playlist_name);
    QVector<Song> lsDirsSongs(const QStringList &paths);
    bool appendSongsToPlaylist(const QStringList &paths, const QString &playlist_name);
    bool removeSongsFromPlaylist(const QVector<int> &indecies, const QString &playlist_name);

  signals:
    void connected(const QUrl &url);
    void disconnected(const QUrl &url);
    void error(const QUrl &url);
    void databaseUpdated();
    void playlistUpdated();
    void playerStateChanged();
    void mixerChanged();

  private slots:
    void on_idle_readable();

  private:
    struct mpd_connection *conn;
    struct mpd_connection *idle_conn;
    QSocketNotifier *idle_notifier;
    QUrl current_connection_url;
    QTimer conn_timer;

    bool establish_idle(const QUrl &url);
    void deestablish();
    void waitConnected();
    QString lastError() const;
  };
}

#endif // MPD_CLIENT_CONNECTION_H
