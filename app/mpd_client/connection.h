#ifndef MPD_CLIENT_CONNECTION_H
#define MPD_CLIENT_CONNECTION_H

#include "entity.h"
#include "status.h"
#include "song.h"
#include "output.h"

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
    explicit Connection();
    ~Connection();

  public slots:
    bool open(const QUrl &url);
    void close();
    QPair<bool, QString> probe(const QUrl &url);
    bool ping();
    QUrl currentUrl() const;

    QVector<MpdClient::Entity> lsDir(const QString &path);
    MpdClient::Status status();
    QVector<MpdClient::Song> lsPlaylistSongs(const QString &playlist_name);
    QVector<MpdClient::Song> lsDirsSongs(const QStringList &paths);
    bool appendSongsToPlaylist(const QStringList &paths, const QString &playlist_name);
    bool removeSongsFromPlaylist(const QVector<int> &indecies, const QString &playlist_name);
    QVector<MpdClient::Entity> playlists();
    bool removePlaylist(const QString &playlist_name);
    bool createPlaylist(const QStringList &song_paths, const QString &playlist_name);
    bool play(const QString &playlist_name, int position);
    bool pause();
    bool unpause();
    bool stop();
    bool next();
    bool prev();
    Song currentSong();
    bool setVolume(int volume);
    bool setPosition(int pos);
    bool setRepeat(bool repeat);
    bool setRandom(bool rand);
    QVector<MpdClient::Output> outputs();
    bool changeOutputState(int outid, bool state);
    QVector<MpdClient::Song> lsQueueSongs();
    bool setPriority(int song_id, int prio);
    bool resetAllPriorities();
    bool updateDb();
    QByteArray albumArt(const QString& filepath);
    QByteArray readPicture(const QString& filepath);
    bool renamePlaylist(const QString &old_name, const QString &new_name);

  signals:
    void connected(const QUrl &url);
    void disconnected(const QUrl &url);
    void error(const QUrl &url, const QString &message = "");
    void idleEvent(mpd_idle event);

  private slots:
    void on_idle_readable();
    void on_timeout();

  private:
    struct mpd_connection *conn = nullptr;
    struct mpd_connection *idle_conn = nullptr;
    QSocketNotifier *idle_notifier = nullptr;
    QUrl current_connection_url;
    QTimer *conn_timer = nullptr;

    bool establish_idle(const QUrl &url);
    void destroy();
    void waitConnected();
    QString lastError() const;
    void initConnTimer();
  };
}

Q_DECLARE_METATYPE(QVector<MpdClient::Entity>)
Q_DECLARE_METATYPE(QVector<MpdClient::Output>)
Q_DECLARE_METATYPE(QVector<MpdClient::Song>)

#endif // MPD_CLIENT_CONNECTION_H
