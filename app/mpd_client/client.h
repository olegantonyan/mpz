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

    QPair<bool, QString> probe(const QUrl &url);

  public slots:
    void openConnection(const QUrl &url);
    void closeConnection();
    QUrl currentUrl() const;
    bool ping();
    QVector<Entity> lsDir(const QString &path);
    Status status();
    QVector<Song> lsPlaylistSongs(const QString &playlist_name);
    QVector<Song> lsDirsSongs(const QStringList &paths);
    bool appendSongsToPlaylist(const QStringList &paths, const QString &playlist_name);
    bool removeSongsFromPlaylist(const QVector<int> &indecies, const QString &playlist_name);
    QVector<Entity> playlists();
    bool removePlaylist(const QString &playlist_name);
    bool createPlaylist(const QStringList &paths, const QString &playlist_name);
    void play(const QString &playlist_name, int position);
    void pause();
    void unpause();
    void stop();
    void next();
    void prev();
    Song currentSong();

  signals:
    void connected(const QUrl &url);
    void disconnected(const QUrl &url);
    void error(const QUrl &url);

    void databaseUpdated();
    void playlistUpdated();
    void playerStateChanged();
    void mixerChanged();

  private:
    QThread *thread;
    Connection *conn;

  private slots:
    void on_idleEvent(mpd_idle event);
  };
}

#endif // MPD_CLIENT_CLIENT_H
