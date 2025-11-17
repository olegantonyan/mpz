#include "mpdconnection.h"

#include <QDebug>
#include <QEventLoop>

MpdConnection::MpdConnection(QObject *parent) : QObject{parent}, conn(nullptr), idle_conn(nullptr), idle_notifier(nullptr) {
  conn_timer.setInterval(1500);
  connect(&conn_timer, &QTimer::timeout, [=] {
    if (currentUrl().isEmpty()) {
      return;
    }
    if (!ping()) {
      qWarning() << "mpd connection lost with" << currentUrl();
      emit lost();
      conn_timer.stop();
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
    qWarning() << "mpd_run_status failed:" << lastError();
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
  qDebug() << "connecting to mpd at" << url;
  destroy();
  conn = mpd_connection_new(url.host().toUtf8().constData(), url.port(), 0);
  if (!conn) {
    qWarning() << "error allocation mpd connection";
    emit failed();
    return false;
  }
  if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
    qWarning() << "error connecting to mpd:" << lastError();
    mpd_connection_free(conn);
    conn = nullptr;
    emit failed();
    return false;
  }
  if (!establish_idle(url)) {
    mpd_connection_free(conn);
    conn = nullptr;
    emit failed();
    return false;
  }
  current_connection_url = url;
  emit connected(url);
  conn_timer.start();
  return true;
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
  mpd_send_idle(idle_conn);
}

void MpdConnection::destroy() {
  bool should_emit = !!conn;
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
  current_connection_url = QUrl();
  conn_timer.stop();
  if (should_emit) {
    emit disconnected();
  }
}

MpdConnection::~MpdConnection() {
  destroy();
}
