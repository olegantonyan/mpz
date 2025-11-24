#include "connection.h"

#include <QDebug>
#include <QEventLoop>

class TimerStarter {
public:
  explicit TimerStarter(QTimer &tmr) : timer(tmr) { timer.stop(); }
  ~TimerStarter() { timer.start(); }
private:
  QTimer &timer;
};

namespace MpdClient {
Connection::Connection(QThread *thread) : QObject{nullptr} {
    moveToThread(thread);

    conn = nullptr;
    idle_conn = nullptr;
    idle_notifier = nullptr;

    conn_timer.moveToThread(thread);
    conn_timer.setInterval(3210);
    connect(&conn_timer, &QTimer::timeout, [=] {
      if (currentUrl().isEmpty()) {
        return;
      }
      if (!ping()) {
        qWarning() << "mpd connection lost with" << currentUrl();
        emit error(currentUrl());
        emit disconnected(currentUrl());
        open(currentUrl());
      }
    });
  }

  QString Connection::lastError() const {
    return QString::fromUtf8(mpd_connection_get_error_message(conn));
  }

  QUrl Connection::currentUrl() const {
    return current_connection_url;
  }

  QVector<Entity> Connection::lsDir(const QString &path) {
    QVector<Entity> result;

    if (!conn) {
      return result;
    }

    if (!mpd_send_list_meta(conn, path.toUtf8().constData())) {
      qWarning() << "mpd_send_list_all:" << lastError();
      return result;
    }

    struct mpd_entity* entity;
    while ((entity = mpd_recv_entity(conn)) != nullptr) {
      result.append(Entity(entity));
      mpd_entity_free(entity);
    }
    mpd_response_finish(conn);

    return result;
  }

  Status Connection::status() {
    Status result;

    if (!conn) {
      return result;
    }

    mpd_status *status = mpd_run_status(conn);
    if (!status) {
      qWarning() << "mpd_run_status: " << lastError();
      return result;
    }

    result = Status(status);

    return result;
  }

  QVector<Song> Connection::lsPlaylistSongs(const QString &playlist_name) {
    QVector<Song> result;

    if (!mpd_send_list_playlist_meta(conn, playlist_name.toUtf8().constData())) {
      qWarning() << "mpd_send_list_playlist_meta:" << lastError();
      return result;
    }

    struct mpd_song *song;
    while ((song = mpd_recv_song(conn)) != nullptr) {
      result.append(Song(song));
      mpd_song_free(song);
    }

    mpd_response_finish(conn);

    return result;
  }

  QVector<Song> Connection::lsDirsSongs(const QStringList &paths) {
    QVector<Song> result;

    for (auto path : paths) {
      if (!mpd_send_list_all_meta(conn, path.toUtf8().constData())) {
        qWarning() << "mpd_send_list_all_meta: " << lastError();
        mpd_response_finish(conn);
        return result;
      }

      struct mpd_song *song;
      while ((song = mpd_recv_song(conn)) != nullptr) {
        result.append(Song(song));
        mpd_song_free(song);
      }

      mpd_response_finish(conn);
    }

    return result;
  }

  bool Connection::appendSongsToPlaylist(const QStringList &paths, const QString &playlist_name) {
    if (!mpd_command_list_begin(conn, true)) {
      qWarning() << "mpd_send_list_all_meta: " << lastError();
      return false;
    }

    bool ok = true;
    for (auto path : paths) {
      if (!mpd_send_playlist_add(conn, playlist_name.toUtf8().constData(), path.toUtf8().constData())) {
        qWarning() << "mpd_send_playlist_add: " << lastError();
        ok = false;
      }
    }

    if (!mpd_command_list_end(conn)) {
      qWarning() << "mpd_send_playlist_add: " << lastError();
      ok = false;
    }

    if (!mpd_response_finish(conn)) {
      qWarning() << "mpd_send_playlist_add: " << lastError();
      ok = false;
    }
    return ok;
  }

  bool Connection::removeSongsFromPlaylist(const QVector<int> &indecies, const QString &playlist_name) {
    if (!mpd_command_list_begin(conn, true)) {
      qWarning() << "mpd_command_list_begin:" << lastError();
      mpd_response_finish(conn);
      return false;
    }

    for (int i : indecies) {
      if (!mpd_send_playlist_delete(conn, playlist_name.toUtf8().constData(), i)) {
        qWarning() << "mpd_command_list_end:" << lastError();
        mpd_response_finish(conn);
        return false;
      }
    }

    if (!mpd_command_list_end(conn)) {
      qWarning() << "mpd_command_list_end:" << lastError();
      return false;
    }
    if (!mpd_response_finish(conn)) {
      qWarning() << "mpd_response_finish:" << lastError();
      return false;
    }
    return true;
  }

  QVector<Entity> Connection::playlists() {
    QVector<Entity> result;

    if (!mpd_send_list_playlists(conn)) {
      qWarning() << "mpd_send_list_playlists:" << lastError();
      return result;
    }

    struct mpd_playlist *pl;
    while ((pl = mpd_recv_playlist(conn)) != nullptr) {
      result.append(Entity(pl));
      mpd_playlist_free(pl);
    }
    mpd_response_finish(conn);

    return result;
  }

  bool Connection::removePlaylist(const QString &playlist_name) {
    if (!mpd_run_rm(conn, playlist_name.toUtf8().constData())) {
      qWarning() << "error deleting mpd playlist:" << lastError();
      return false;
    }
    return true;
  }

  bool Connection::createPlaylist(const QStringList &paths, const QString &playlist_name) {
    auto songs = lsDirsSongs(paths);
    for (auto song : songs) {
      if (!mpd_run_add(conn, song.filepath.toUtf8().constData())) {
        qWarning() << "mpd_run_add: " << lastError();
        return false;
      }
    }
    if (!mpd_run_save(conn, playlist_name.toUtf8().constData())) {
      qWarning() << "mpd_run_save: " << lastError();
      return false;
    }

    return true;
  }

  bool Connection::play(const QString &playlist_name, int position) {
    if (!mpd_run_clear(conn)) {
      qWarning() << "mpd_run_clear:" << lastError();
      return false;
    }
    if (!mpd_run_load(conn, playlist_name.toUtf8().constData())) {
      qWarning() << "mpd_run_load:" << lastError();
      return false;
    }

    if (!mpd_run_idle_mask(conn, MPD_IDLE_QUEUE)) {
      qWarning() << "mpd_run_idle_mask:" << lastError();
      return false;
    }

    if (!mpd_run_play_pos(conn, position)) {
      qWarning() << "mpd_run_play_pos:" << lastError();
      return false;
    }

    return true;
  }

  bool Connection::pause() {
    if (!mpd_run_pause(conn, true)) {
      qWarning() << "mpd_run_pause:" << lastError();
      return false;
    }
    return true;
  }

  bool Connection::unpause() {
    if (!mpd_run_pause(conn, false)) {
      qWarning() << "mpd_run_pause:" << lastError();
      return false;
    }
    return true;
  }

  bool Connection::stop() {
    if (!mpd_run_stop(conn)) {
      qWarning() << "mpd_run_stop:" << lastError();
      return false;
    }
    return true;
  }

  bool Connection::next() {
    if (!mpd_run_next(conn)) {
      qWarning() << "mpd_run_next:" << lastError();
      return false;
    }
    return true;
  }

  bool Connection::prev() {
    if (!mpd_run_previous(conn)) {
      qWarning() << "mpd_run_previous:" << lastError();
      return false;
    }
    return true;
  }

  void Connection::waitConnected() {
    if (!ping()) {
      QEventLoop loop;
      connect(this, &Connection::connected, &loop, &QEventLoop::quit);
      loop.exec();
    }
  }

  bool Connection::ping() {
    if (!conn) {
      return false;
    }
    struct mpd_status *status = mpd_run_status(conn);
    if (!status) {
      return false;
    }
    mpd_status_free(status);
    return true;
  }

  bool Connection::open(const QUrl &url) {
    if (url.isEmpty()) {
      return false;
    }

    current_connection_url = url;
    TimerStarter tmr(conn_timer);

    destroy();
    auto new_conn = mpd_connection_new(url.host().toUtf8().constData(), url.port(), 0);
    if (!new_conn) {
      qWarning() << "error allocation mpd connection";
      emit error(url);
      return false;
    }
    if (mpd_connection_get_error(new_conn) != MPD_ERROR_SUCCESS) {
      qWarning() << "error connecting to mpd:" << QString::fromUtf8(mpd_connection_get_error_message(new_conn));
      mpd_connection_free(new_conn);
      emit error(url);
      return false;
    }
    if (!establish_idle(url)) {
      mpd_connection_free(new_conn);
      emit error(url);
      return false;
    }
    qDebug() << "connected to mpd at" << url;
    conn = new_conn;
    emit connected(url);
    return true;
  }

  void Connection::close() {
    destroy();
    auto url = current_connection_url;
    current_connection_url = QUrl();
    conn_timer.stop();
    emit disconnected(url);
  }

  QPair<bool, QString> Connection::probe(const QUrl &url) {
    QPair<bool, QString> result(true, "");

    auto probed_conn = mpd_connection_new(url.host().toUtf8().constData(), url.port(), 0);
    if (!probed_conn) {
      result.first = false;
      result.second = QString::fromUtf8(mpd_connection_get_error_message(probed_conn));
      return result;
    }
    if (mpd_connection_get_error(probed_conn) != MPD_ERROR_SUCCESS) {
      result.first = false;
      result.second = QString::fromUtf8(mpd_connection_get_error_message(probed_conn));
      mpd_connection_free(probed_conn);
      return result;
    }
    const unsigned int *ver = mpd_connection_get_server_version(probed_conn);
    auto version = QString("protocol version %1.%2.%3").arg(ver[0]).arg(ver[1]).arg(ver[2]);

    if (!mpd_send_stats(probed_conn)) {
      result.first = false;
      result.second = QString::fromUtf8(mpd_connection_get_error_message(probed_conn));
      mpd_connection_free(probed_conn);
      return result;
    }

    struct mpd_stats *stats = mpd_recv_stats(probed_conn);
    if (!stats) {
      result.first = false;
      result.second = QString::fromUtf8(mpd_connection_get_error_message(probed_conn));
      mpd_connection_free(probed_conn);
      return result;
    }

    unsigned int songs = mpd_stats_get_number_of_songs(stats);
    mpd_stats_free(stats);
    mpd_response_finish(probed_conn);
    mpd_connection_free(probed_conn);

    result.second = QString("%1, %2 songs").arg(version).arg(songs);
    return result;
  }

  bool Connection::establish_idle(const QUrl &url) {
    if (url.isEmpty()) {
      return false;
    }

    idle_conn = mpd_connection_new(url.host().toUtf8().constData(), url.port(), 0);
    if (!idle_conn) {
      qWarning() << "error allocation mpd idle connection";
      return false;
    }
    if (mpd_connection_get_error(idle_conn) != MPD_ERROR_SUCCESS) {
      qWarning() << "error establishing idle connection" << mpd_connection_get_error_message(idle_conn);
      return false;
    }
    mpd_send_idle(idle_conn);

    int fd = mpd_connection_get_fd(idle_conn);
    idle_notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(idle_notifier, &QSocketNotifier::activated, this, &Connection::on_idle_readable);
    return true;
  }

  void Connection::on_idle_readable() {
    enum mpd_idle event = mpd_recv_idle(idle_conn, false);
    emit idleEvent(event);
    mpd_send_idle(idle_conn);
  }

  void Connection::destroy() {
    if (conn) {
      mpd_connection_free(conn);
      conn = nullptr;
    }
    if (idle_conn) {
      mpd_connection_free(idle_conn);
      idle_conn = nullptr;
    }
    if (idle_notifier) {
      delete idle_notifier;
      idle_notifier = nullptr;
    }
  }

  Connection::~Connection() {
    close();
  }
}
