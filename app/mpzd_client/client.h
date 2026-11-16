#ifndef MPZD_CLIENT_CLIENT_H
#define MPZD_CLIENT_CLIENT_H

#include "mpzd_client/protocol.h"

#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QQueue>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>

#include <functional>

namespace MpzdClient {
  class Client : public QObject {
    Q_OBJECT
  public:
    using Callback = std::function<void(const Reply &)>;

    explicit Client(QObject *parent = nullptr);
    ~Client() override;

    static QPair<bool, QString> probe(const QUrl &url, int timeout_ms = 2000);

    QUrl currentUrl() const;
    bool isConnected() const;
    const Status &status() const;

    void request(const QStringList &args, QObject *context = nullptr, Callback callback = nullptr);
    void fetchTracks(quint32 playlist_id, const QString &playlist_name, QObject *context, std::function<void(bool, const QVector<Track> &)> callback);

  public slots:
    void openConnection(const QUrl &url);
    void closeConnection();
    void refreshStatus();

  signals:
    void connected(const QUrl &url);
    void disconnected(const QUrl &url);
    void error(const QUrl &url, const QString &message);
    void statusChanged(const MpzdClient::Status &status);
    void playlistsChanged();
    void playlistChanged(quint32 id);
    void scanDone(quint32 id);

  private:
    struct Pending {
      QPointer<QObject> context;
      bool has_context;
      Callback callback;
      QElapsedTimer sent;
    };

    QTcpSocket socket;
    QUrl url;
    Parser parser;
    QQueue<Pending> pending;
    QTimer reconnect_timer;
    QTimer connect_timer;
    QTimer watchdog_timer;
    QElapsedTimer last_rx;
    Status last_status;
    bool ready = false;
    bool error_reported = false;
    bool status_requested = false;

    void connectSocket();
    void abortSocket();
    void dropConnection(const QString &message);
    void onWatchdog();
    void failPending(const QString &message);
    void handleEvent(const QString &event);
    void onReadyRead();
    void onSocketClosed();
    void onSocketError();
  };
}

#endif // MPZD_CLIENT_CLIENT_H
