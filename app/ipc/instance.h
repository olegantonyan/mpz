#ifndef IPC_INSTANCE_H
#define IPC_INSTANCE_H

#include <QObject>
#include <QVariant>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QUrl>
#include <QVariantMap>

namespace IPC {
  class Instance : public QObject {
    Q_OBJECT
  public:
    explicit Instance(int timeout_ms = 800, int port = 10001, QObject *parent = nullptr);

    bool isAnotherRunning() const;

  public slots:
    bool send(const QVariantMap &data) const;
    void start();

  signals:
    void received(const QVariantMap &data);

  private:
    const int timeout_ms;
    const int port;
    QTcpServer server;

    QByteArray response() const;
    void process(const QByteArray &request);
    QUrl url() const;

  private slots:
    void on_server_connection();
  };
}

#endif // IPC_INSTANCE_H
