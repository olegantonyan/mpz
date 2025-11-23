#include "mpdconnection.h"

#include <QDebug>
#include <QEventLoop>

class TimerStarter {
public:
  explicit TimerStarter(QTimer &tmr) : timer(tmr) { timer.stop(); }
  ~TimerStarter() { timer.start(); }
private:
  QTimer &timer;
};

MpdConnection::MpdConnection(QObject *parent) : QObject{parent}, conn(nullptr), idle_conn(nullptr), idle_notifier(nullptr) {
  conn_timer.setInterval(3210);
  connect(&conn_timer, &QTimer::timeout, [=] {
    if (currentUrl().isEmpty()) {
      return;
    }
    if (!ping()) {
      qWarning() << "mpd connection lost with" << currentUrl();
      emit error(currentUrl());
      emit disconnected(currentUrl());
      establish(currentUrl());
    }
  });
}

QString MpdConnection::lastError() const {
  return QString::fromUtf8(mpd_connection_get_error_message(conn));
}

QUrl MpdConnection::currentUrl() const {
  return current_connection_url;
}

void MpdConnection::waitConnected() {
  if (!ping()) {
    QEventLoop loop;
    connect(this, &MpdConnection::connected, &loop, &QEventLoop::quit);
    loop.exec();
  }
}

bool MpdConnection::ping() {
  QMutexLocker locker(&mutex);
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

bool MpdConnection::establish(const QUrl &url) {
  if (url.isEmpty()) {
    return false;
  }

  QMutexLocker locker(&mutex);
  current_connection_url = url;
  TimerStarter tmr(conn_timer);

  deestablish();
  conn = mpd_connection_new(url.host().toUtf8().constData(), url.port(), 0);
  if (!conn) {
    qWarning() << "error allocation mpd connection";
    emit error(url);
    return false;
  }
  if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
    qWarning() << "error connecting to mpd:" << lastError();
    mpd_connection_free(conn);
    conn = nullptr;
    emit error(url);
    return false;
  }
  if (!establish_idle(url)) {
    mpd_connection_free(conn);
    conn = nullptr;
    emit error(url);
    return false;
  }
  qDebug() << "connected to mpd at" << url;
  emit connected(url);
  return true;
}

QPair<bool, QString> MpdConnection::probe(const QUrl &url) {
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

bool MpdConnection::establish_idle(const QUrl &url) {
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
  connect(idle_notifier, &QSocketNotifier::activated, this, &MpdConnection::on_idle_readable);
  return true;
}

void MpdConnection::deestablish() {
  QMutexLocker locker(&mutex);

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

void MpdConnection::on_idle_readable() {
  enum mpd_idle event = mpd_recv_idle(idle_conn, false);
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
  mpd_send_idle(idle_conn);
}

void MpdConnection::destroy() {
  deestablish();
  auto url = current_connection_url;
  current_connection_url = QUrl();
  conn_timer.stop();
  emit disconnected(url);
}

MpdConnection::~MpdConnection() {
  destroy();
}
