#include "mpdconnection.h"

#include <QDebug>

MpdConnection::MpdConnection(QObject *parent) : QObject{parent}, conn(nullptr), idle_conn(nullptr), idle_notifier(nullptr) {
}

QString MpdConnection::last_error() const {
  return QString::fromUtf8(mpd_connection_get_error_message(conn));
}

bool MpdConnection::ping() {
  QMutexLocker locker(&mutex);
  if (!conn) {
    return false;
  }
  struct mpd_status *status = mpd_run_status(conn);
  if (!status) {
    qWarning() << "mpd connection failed" << last_error();
    return false;
  }
  mpd_status_free(status);
  return true;
}

bool MpdConnection::establish(const QUrl &url) {
  QMutexLocker locker(&mutex);
  qDebug() << "connecting to mpd at" << url;
  destroy();
  conn = mpd_connection_new(url.host().toUtf8().constData(), url.port(), 0);
  if (!conn) {
    qWarning() << "error allocation mpd connection";
    return false;
  }
  if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
    qWarning() << "error connecting to mpd:" << last_error();
    mpd_connection_free(conn);
    return false;
  }
  establish_idle(url);
  emit connected(this);
  return true;
}

bool MpdConnection::establish_idle(const QUrl &url) {
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
    qDebug() << "mpd database changed";
  }
  if (event & MPD_IDLE_PLAYER) {
    qDebug() << "mpd player state changed";
  }
  if (event & MPD_IDLE_STORED_PLAYLIST) {
    qDebug() << "mpd playlist changed";
  }
  mpd_send_idle(idle_conn);
}

void MpdConnection::destroy() {
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
  }
}

MpdConnection::~MpdConnection() {
  destroy();
}
