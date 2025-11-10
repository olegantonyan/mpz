#include "mpdconnection.h"

#include <QDebug>

MpdConnection::MpdConnection(QObject *parent) : QObject{parent}, connection(nullptr) {
}

struct mpd_connection *MpdConnection::get() const {
  return connection;
}

struct mpd_connection *MpdConnection::establish(const QUrl &url) {
  qDebug() << "connecting to mpd at" << url;
  destroy();
  connection = mpd_connection_new(url.host().toUtf8().constData(), url.port(), 0);
  if (!connection) {
    qWarning() << "error allocation mpd connection";
    return nullptr;
  }
  if (mpd_connection_get_error(connection) != MPD_ERROR_SUCCESS) {
    qWarning() << "error connecting to mpd:" << mpd_connection_get_error_message(connection);
    mpd_connection_free(connection);
    return nullptr;
  }
  emit connected(this);
  return get();
}

void MpdConnection::destroy() {
  if (connection) {
    mpd_connection_free(connection);
  }
  connection = nullptr;
}

MpdConnection::~MpdConnection() {
  destroy();
}
