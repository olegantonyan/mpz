#ifndef IPC_INSTANCE_H
#define IPC_INSTANCE_H

#include <QObject>
#include <QVariant>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>

namespace IPC {
  class Instance : public QObject {
    Q_OBJECT
  public:
    explicit Instance(QObject *parent = nullptr);

    bool isAnotherRunning() const;

  public slots:
    void send(const QVariant &data);

  signals:
    void received(const QVariant &data);

  private:
    QTcpServer server;

    QByteArray response(bool ok) const;
    bool process(const QByteArray &request);

  private slots:
    void on_server_connection();
  };
}

#endif // IPC_INSTANCE_H
