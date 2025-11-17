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
  QString lastError() const;
  void waitConnected();
  QUrl currentUrl() const;

signals:
  void connected();
  void databaseUpdated();
  void playlistUpdated();
  void playerStateChanged();

private slots:
  void on_idle_readable();

private:
  QRecursiveMutex mutex;
  struct mpd_connection *idle_conn;
  QSocketNotifier *idle_notifier;
  QUrl current_connection_url;

  bool establish_idle(const QUrl &url);

  friend class MpdConnectionLocker;
};

class MpdConnectionLocker {
public:
  explicit MpdConnectionLocker(MpdConnection &c) : locker(nullptr) {
    c.waitConnected();
    locker = new QMutexLocker<QRecursiveMutex>(&c.mutex);
  }
  ~MpdConnectionLocker() {
    delete locker;
  }
private:
  QMutexLocker<QRecursiveMutex> *locker;
};

#endif // MPDCONNECTION_H
