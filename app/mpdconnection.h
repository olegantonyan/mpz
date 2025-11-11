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
  QRecursiveMutex mutex;

  bool establish(const QUrl &url);
  bool ping();
  void destroy();
  QString last_error() const;

signals:
  void connected(MpdConnection *self);

private slots:
  void on_idle_readable();

private:
  struct mpd_connection *idle_conn;
  QSocketNotifier *idle_notifier;

  bool establish_idle(const QUrl &url);
};

#endif // MPDCONNECTION_H
