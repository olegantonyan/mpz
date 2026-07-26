#ifndef IPC_INSTANCE_H
#define IPC_INSTANCE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QByteArray>
#include <QLocalServer>

namespace IPC {
  class Instance : public QObject {
    Q_OBJECT
  public:
    explicit Instance(int timeout_ms = 500, QObject *parent = nullptr);

    int anotherPid() const;

  public slots:
    int send(const QVariantMap &data) const;
    bool start();
    bool load_files_send(const QStringList &list);

  signals:
    void load_files_received(const QStringList &list);

  private:
    const int timeout_ms;
    const QString socket_name;
    QLocalServer server;

    static QString socketName();
    QByteArray process_received(const QByteArray &request);

  private slots:
    void on_server_connection();
  };
}

#endif // IPC_INSTANCE_H
