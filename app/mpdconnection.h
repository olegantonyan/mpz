#ifndef MPDCONNECTION_H
#define MPDCONNECTION_H

#include <QObject>
#include <QUrl>

#include "mpd/client.h"

class MpdConnection : public QObject {
  Q_OBJECT
public:
  explicit MpdConnection(QObject *parent = nullptr);
  ~MpdConnection();

  struct mpd_connection *get() const;
  struct mpd_connection *establish(const QUrl &url);
  void destroy();

signals:
  void connected(MpdConnection *self);

private:
  struct mpd_connection *connection;
};

#endif // MPDCONNECTION_H
