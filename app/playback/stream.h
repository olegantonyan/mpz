#ifndef PLAYBACK_STREAM_H
#define PLAYBACK_STREAM_H

#include <QObject>
#include <QIODevice>
#include <QUrl>
#include <QMutex>
#include <QFuture>

namespace Playback {
  class Stream : public QIODevice {
    Q_OBJECT
  public:
    explicit Stream(quint32 threshold_bytes = 131072, QObject *parent = nullptr);
    ~Stream() override;

    bool start(quint32 timeout_ms = 60000);

    bool waitForFill(quint32 timeout_ms);

    bool isRunning() const;
    bool isValidUrl() const;

    void setUrl(const QUrl &url);
    QUrl url() const;

    // QIODevice interface
    bool isSequential() const override;
    qint64 pos() const override;
    qint64 bytesAvailable() const override;

  public slots:
    void stop();

  signals:
    void stopped();
    void stopping();
    void started();
    void fillChanged(quint32 current, quint32 total);
    void error(const QString& message);

  private:
    QByteArray _buffer;
    qint64 _total_bytes_received;
    QUrl _url;
    const quint32 _threshold_bytes;
    const quint32 _max_bytes;
    QMutex _mutex;
    QFuture<void> _future;

    void append(const QByteArray& a);
    void clear();

    void thread();

  protected:
    // QIODevice interface
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;
  };
}

#endif // PLAYBACK_STREAM_H
