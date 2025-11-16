#ifndef MPDCONNECTION_H
#define MPDCONNECTION_H

#include <QObject>
#include <QUrl>
#include <QString>
#include <QSocketNotifier>
#include <QRecursiveMutex>
#include <QMutexLocker>

#include "mpd/client.h"

class MpdConnection : public QObject {
  Q_OBJECT
public:
  explicit MpdConnection(QObject *parent = nullptr);
  ~MpdConnection();

  struct mpd_connection *conn;

  bool establish(const QUrl &url);
  bool ping();
  void destroy();
  QString last_error() const;
  void wait_connected();

signals:
  void connected();
  void database_updated();
  void playlist_updated();
  void player_state_changed();

private slots:
  void on_idle_readable();

private:
  QRecursiveMutex mutex;
  struct mpd_connection *idle_conn;
  QSocketNotifier *idle_notifier;

  bool establish_idle(const QUrl &url);

  friend class MpdConnectionLocker;
};

class MpdConnectionLocker {
public:
  explicit MpdConnectionLocker(MpdConnection &c) : locker(nullptr) {
    c.wait_connected();
    locker = new QMutexLocker<QRecursiveMutex>(&c.mutex);
  }
  ~MpdConnectionLocker() {
    delete locker;
  }
private:
  QMutexLocker<QRecursiveMutex> *locker;
};

#endif // MPDCONNECTION_H
