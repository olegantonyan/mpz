#ifndef PLAYBACK_STREAM_H
#define PLAYBACK_STREAM_H

#include "streammetadata.h"

#include <QObject>
#include <QIODevice>
#include <QUrl>
#include <QMutex>
#include <QFuture>
#include <QStringList>
#include <QMap>

namespace Playback {
  class Stream : public QIODevice {
    Q_OBJECT
  public:
    explicit Stream(quint32 threshold_bytes, quint16 threshold_multiplier = 32, QObject *parent = nullptr);
    ~Stream() override;

    bool start();

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
    void metadataChanged(const StreamMetaData& meta);

  private:
    QByteArray _buffer;
    qint64 _total_bytes_received;
    QUrl _url;
    const quint32 _threshold_bytes;
    const quint32 _max_bytes;
    mutable QMutex _mutex;
    QFuture<void> _future;
    int _icy_metaint;
    qint64 _next_meta_pos;
    QByteArray _meta_pending;
    int _meta_bytes_remaining;
    StreamMetaData _meta;
    int _timeout_ms;
    // chunked transfer-encoding decode state. HTTP/1.1 proxies in front of the
    // shoutcast/icecast server (e.g. Caddy) frame the body as chunked; we request
    // HTTP/1.0 to avoid it, but still decode it here as a safety net.
    bool _chunked;
    enum class ChunkState { Size, Data, AfterData, Done } _chunk_state;
    qint64 _chunk_remaining;
    QByteArray _chunk_size_line;

    void feed(const QByteArray& raw);
    QByteArray dechunk(const QByteArray& in);
    void append(const QByteArray& a);
    void clear();
    void thread();
    bool extract_icy_metaint(const QMap<QString, QString> &headers);
    int append_extract_meta(const QByteArray &a);
    QString buildRequest() const;
    QMap<QString, QString> parseHeaders(QStringList rawheaders);

  protected:
    // QIODevice interface
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;
  };
}

#endif // PLAYBACK_STREAM_H
