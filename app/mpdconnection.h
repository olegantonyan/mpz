#ifndef MPDCONNECTION_H
#define MPDCONNECTION_H

#include <QObject>
#include <QUrl>
#include <QString>
#include <QSocketNotifier>
#include <QRecursiveMutex>
#include <QMutexLocker>
#include <QTimer>

#include "mpd/client.h"

class MpdConnection : public QObject {
  Q_OBJECT
public:
  explicit MpdConnection(QObject *parent = nullptr);
  ~MpdConnection();

  struct mpd_connection *conn;

  bool establish(const QUrl &url);
  QPair<bool, QString> probe(const QUrl &url);
  bool ping();
  void destroy();
  QString lastError() const;
  void waitConnected();
  QUrl currentUrl() const;

signals:
  void connected(const QUrl &url);
  void disconnected(const QUrl &url);
  void error(const QUrl &url);
  void databaseUpdated();
  void playlistUpdated();
  void playerStateChanged();
  void mixerChanged();

private slots:
  void on_idle_readable();

private:
  QRecursiveMutex mutex;
  struct mpd_connection *idle_conn;
  QSocketNotifier *idle_notifier;
  QUrl current_connection_url;
  QTimer conn_timer;

  bool establish_idle(const QUrl &url);
  void deestablish();

  friend class MpdConnectionLocker;
};

class MpdConnectionLocker {
public:
  explicit MpdConnectionLocker(MpdConnection &c) : locker(nullptr) {
    c.waitConnected();
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    locker = new QMutexLocker<QRecursiveMutex>(&c.mutex);
#else
    locker = new QMutexLocker(&c.mutex);
#endif

  }
  ~MpdConnectionLocker() {
    delete locker;
  }
private:
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    QMutexLocker<QRecursiveMutex> *locker;
#else
    QMutexLocker*locker;
#endif
};

#endif // MPDCONNECTION_H
