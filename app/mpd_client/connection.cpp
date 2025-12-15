#include "connection.h"

#include <QDebug>
#include <QEventLoop>

#define MPD_TIMEOUT 8000

class TimerStarter {
public:
  explicit TimerStarter(QTimer *tmr) : timer(tmr) {
    timer->stop();
  }
  ~TimerStarter() {
    timer->start();
  }
private:
  QTimer *timer;
};

namespace MpdClient {
  Connection::Connection() : QObject{nullptr} {
  }

  QString Connection::lastError() const {
    if (!conn) {
      return "";
    }
    return QString::fromUtf8(mpd_connection_get_error_message(conn));
  }

  void Connection::initConnTimer() {
    if (conn_timer) {
      return;
    }
    conn_timer = new QTimer(this);
    conn_timer->setInterval(MPD_TIMEOUT + 500);
    connect(conn_timer, &QTimer::timeout, this, &Connection::on_timeout);
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

  MpdClient::Status Connection::status() {
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
    if (!conn) {
      return result;
    }

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
    if (!conn) {
      return result;
    }

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
    if (!conn) {
      return false;
    }

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
    if (!conn) {
      return false;
    }

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

  QVector<MpdClient::Entity> Connection::playlists() {
    QVector<Entity> result;
    if (!conn) {
      return result;
    }

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
    if (!conn) {
      return false;
    }

    if (!mpd_run_rm(conn, playlist_name.toUtf8().constData())) {
      qWarning() << "error deleting mpd playlist:" << lastError();
      return false;
    }
    return true;
  }

  bool Connection::createPlaylist(const QStringList &song_paths, const QString &playlist_name) {
    if (!conn) {
      return false;
    }

    if (!mpd_run_save(conn, playlist_name.toUtf8().constData())) {
      qWarning() << "mpd_run_save: " << playlist_name << lastError();
      return false;
    }
    if (!mpd_run_playlist_clear(conn, playlist_name.toUtf8().constData())) {
      qWarning() << "mpd_run_playlist_clear: " << playlist_name << lastError();
      return false;
    }
    for (auto path : song_paths) {
      if (!mpd_run_playlist_add(conn, playlist_name.toUtf8().constData(), path.toUtf8().constData())) {
        qWarning() << "mpd_run_playlist_add: " << path << lastError();
        return false;
      }
    }

    /*if (!mpd_run_clear(conn)) {
      qWarning() << "mpd_run_clear: " << lastError();
      return false;
    }

    for (auto path : song_paths) {
      if (!mpd_run_add(conn, path.toUtf8().constData())) {
        qWarning() << "mpd_run_add: " << lastError();
        return false;
      }
    }
    if (!mpd_run_save(conn, playlist_name.toUtf8().constData())) {
      qWarning() << "mpd_run_save: " << lastError();
      return false;
    }*/

    return true;
  }

  bool Connection::play(const QString &playlist_name, int position) {
    if (!conn) {
      return false;
    }

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
    if (!conn) {
      return false;
    }

    if (!mpd_run_pause(conn, true)) {
      qWarning() << "mpd_run_pause:" << lastError();
      return false;
    }
    return true;
  }

  bool Connection::unpause() {
    if (!conn) {
      return false;
    }

    if (!mpd_run_pause(conn, false)) {
      qWarning() << "mpd_run_pause:" << lastError();
      return false;
    }
    return true;
  }

  bool Connection::stop() {
    if (!conn) {
      return false;
    }

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
    if (!conn) {
      return false;
    }

    if (!mpd_run_previous(conn)) {
      qWarning() << "mpd_run_previous:" << lastError();
      return false;
    }
    return true;
  }

  Song Connection::currentSong() {
    Song result;
    if (!conn) {
      return result;
    }

    auto *song = mpd_run_current_song(conn);
    if (song) {
      result = Song(song);
      mpd_song_free(song);
    }

    return result;
  }

  bool Connection::setVolume(int volume) {
    if (!conn) {
      return false;
    }

    if (!mpd_run_set_volume(conn, volume)) {
      qWarning() << "mpd_run_set_volume:" << lastError();
      return false;
    }
    return true;
  }

  bool Connection::setPosition(int pos) {
    if (!conn) {
      return false;
    }

    struct mpd_status *st = mpd_run_status(conn);
    if (!st) {
      return false;
    }
    int id = mpd_status_get_song_id(st);
    mpd_status_free(st);
    if (!mpd_run_seek_id(conn, id, pos)) {
      qWarning() << "mpd_run_seek_id:" << lastError();
      return false;
    }
    return true;
  }

  bool Connection::setRepeat(bool repeat) {
    if (!conn) {
      return false;
    }

    if (!mpd_run_repeat(conn, repeat)) {
      qWarning() << "mpd_run_repeat:" << lastError();
      return false;
    }
    return true;
  }

  bool Connection::setRandom(bool rand) {
    if (!conn) {
      return false;
    }

    if (!mpd_run_random(conn, rand)) {
      qWarning() << "mpd_run_random:" << lastError();
      return false;
    }
    return true;
  }

  QVector<Output> Connection::outputs() {
    QVector<Output> result;
    if (!conn) {
      return result;
    }

    if (!mpd_send_outputs(conn)) {
      qWarning() << "mpd_send_outputs:" << lastError();
      return result;
    }

    struct mpd_output *output;
    while ((output = mpd_recv_output(conn)) != nullptr) {
      result.append(Output(output));
      mpd_output_free(output);
    }
    mpd_response_finish(conn);
    return result;
  }

  bool Connection::changeOutputState(int outid, bool state) {
    bool ok = false;
    if (!conn) {
      return ok;
    }
    if (state) {
      ok = mpd_run_enable_output(conn, outid);
    } else {
      ok = mpd_run_disable_output(conn, outid);
    }
    if (!ok) {
      qWarning() << "mpd_run_enable_output / mpd_run_disable_output:" << lastError();
    }
    return ok;
  }

  bool Connection::setPriority(int song_id, int prio) {
    if (!conn) {
      return false;
    }

    if (!mpd_run_prio_id(conn, prio, song_id)) {
      qWarning() << "mpd_run_prio_pos:" << lastError();
      return false;
    }
    return true;
  }

  bool Connection::resetAllPriorities() {
    if (!conn) {
      return false;
    }

    struct mpd_status *status = mpd_run_status(conn);
    if (status != nullptr) {
      unsigned int queue_len = mpd_status_get_queue_length(status);
      mpd_status_free(status);

      if (queue_len > 0) {
        if (!mpd_run_prio_range(conn, 0, 0, queue_len - 1)) {
          qWarning() << "mpd_run_prio_range:" << lastError();
          return false;
        }
      }
    }
    return true;
  }

  QVector<MpdClient::Song> MpdClient::Connection::lsQueueSongs() {
    QVector<Song> result;
    if (!conn) {
      return result;
    }

    mpd_send_list_queue_meta(conn);

    struct mpd_song *song;
    while ((song = mpd_recv_song(conn)) != nullptr) {
      result << Song(song);
      mpd_song_free(song);
    }
    mpd_response_finish(conn);

    return result;
  }

  bool Connection::updateDb() {
    if (!conn) {
      return false;
    }
    if (!mpd_run_update(conn, nullptr)) {
      qWarning() << "mpd_run_update:" << lastError();
      return false;
    }
    return true;
  }

  QByteArray Connection::albumArt(const QString &filepath) {
    QByteArray result;
    if (!conn) {
      return result;
    }

    static const size_t BUF_SIZE = 1024 * 1024 * 128;
    static char buffer[BUF_SIZE];
    int offset = 0;

    while (true) {
      int n = mpd_run_albumart(conn, filepath.toUtf8().constData(), offset, buffer, BUF_SIZE);
      if (n < 0) {
        mpd_connection_clear_error(conn);
        result.clear();
        return result;
      }
      if (n == 0) {
        break;
      }

      result.append(buffer, n);
      offset += n;
    }

    return result;
  }

  QByteArray MpdClient::Connection::readPicture(const QString &filepath) {
    QByteArray result;
    if (!conn) {
      return result;
    }

    static const size_t BUF_SIZE = 1024 * 1024 * 256;
    static char buffer[BUF_SIZE];
    int offset = 0;

    while (true) {
      int n = mpd_run_readpicture(conn, filepath.toUtf8().constData(), offset, buffer, BUF_SIZE);
      if (n < 0) {
        mpd_connection_clear_error(conn);
        result.clear();
        return result;
      }
      if (n == 0) {
        break;
      }

      result.append(buffer, n);
      offset += n;
    }

    return result;
  }

  bool Connection::renamePlaylist(const QString &old_name, const QString &new_name) {
    if (!conn) {
      return false;
    }

    if (!mpd_run_rename(conn, old_name.toUtf8().constData(), new_name.toUtf8().constData())) {
      qWarning() << "mpd_run_rename:" << lastError();
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
    initConnTimer();
    TimerStarter tmr(conn_timer);

    destroy();
    auto new_conn = mpd_connection_new(url.host().toUtf8().constData(), url.port(), MPD_TIMEOUT);
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
    if (!url.password().isEmpty()) {
      if (!mpd_run_password(new_conn, url.password().toUtf8().constData())) {
        qWarning() << "password auth error:" << QString::fromUtf8(mpd_connection_get_error_message(new_conn));
        mpd_connection_free(new_conn);
        emit error(url);
        return false;
      }
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
    if (conn_timer) {
      conn_timer->stop();
    }
    emit disconnected(url);
  }

  QPair<bool, QString> Connection::probe(const QUrl &url) {
    QPair<bool, QString> result(true, "");

    auto probed_conn = mpd_connection_new(url.host().toUtf8().constData(), url.port(), MPD_TIMEOUT);
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
    if (!url.password().isEmpty()) {
      if (!mpd_run_password(probed_conn, url.password().toUtf8().constData())) {
        result.first = false;
        result.second = QString::fromUtf8(mpd_connection_get_error_message(probed_conn));
        mpd_connection_free(probed_conn);
        return result;
      }
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
    if (!url.password().isEmpty()) {
      if (!mpd_run_password(idle_conn, url.password().toUtf8().constData())) {
        qWarning() << "password auth error idle:" << QString::fromUtf8(mpd_connection_get_error_message(idle_conn));
        mpd_connection_free(idle_conn);
        return false;
      }
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

  void Connection::on_timeout() {
    if (currentUrl().isEmpty()) {
      return;
    }
    if (!ping()) {
      qWarning() << "mpd connection lost with" << currentUrl();
      emit error(currentUrl());
      emit disconnected(currentUrl());
      open(currentUrl());
    }
  }

  void Connection::destroy() {
    if (conn) {
      mpd_response_finish(conn);
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
