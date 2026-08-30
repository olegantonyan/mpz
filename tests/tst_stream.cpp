#include <QtTest>
#include <QElapsedTimer>
#include <QTcpServer>
#include <QTcpSocket>

#include "playback/stream.h"

namespace {
  const int kWaitMs = 10000;

  // Serves one canned response, optionally in pieces, so the split-read paths
  // through parseHeaders/dechunk/append_extract_meta are reachable offline.
  class Server : public QTcpServer {
  public:
    QList<QByteArray> chunks;
    QByteArray request;

    explicit Server(QObject *parent = nullptr) : QTcpServer(parent) {}

    QUrl url() const { return QUrl(QString("http://127.0.0.1:%1/stream").arg(serverPort())); }

  protected:
    void incomingConnection(qintptr handle) override {
      auto *socket = new QTcpSocket(this);
      socket->setSocketDescriptor(handle);
      connect(socket, &QTcpSocket::readyRead, socket, [this, socket]() {
        request.append(socket->readAll());
        if (!request.contains("\r\n\r\n")) {
          return;
        }
        for (const auto &piece : std::as_const(chunks)) {
          socket->write(piece);
          socket->flush();
          socket->waitForBytesWritten(1000);
        }
        // Deliberately kept open: Stream::thread() clears the buffer when the
        // connection ends, so a test has to read while the stream is live.
      });
    }
  };

  QByteArray icyBlock(const QByteArray &title) {
    QByteArray meta = "StreamTitle='" + title + "';";
    const int blocks = (meta.size() + 15) / 16;
    meta.append(QByteArray(blocks * 16 - meta.size(), '\0'));
    return QByteArray(1, static_cast<char>(blocks)) + meta;
  }
}

class TestStream : public QObject {
  Q_OBJECT
private slots:
  void initTestCase();

  void setUrl_fillsInTheImplicitPort();
  void isValidUrl_acceptsHttpSchemesOnly();
  void start_rejectsANonHttpUrl();
  void deliversTheBodyAndTracksTotalBytes();
  void headerNamesAreLowerCasedIntoMetadata();
  void icyMetadataIsStrippedFromTheAudio();
  void icyBlockSplitAcrossReadsIsReassembled();
  void chunkedBodyIsDecoded();
  void chunkedHeaderSplitAcrossReadsIsDecoded();
  void stopClearsTheUrlAndReportsStopped();
  void stopRightAfterStartDoesNotWaitForTheTimeout();

private:
  static QByteArray response(const QByteArray &headers, const QByteArray &body) {
    return "HTTP/1.1 200 OK\r\n" + headers + "\r\n" + body;
  }
  static QByteArray drain(Playback::Stream &stream);
  static QByteArray readAudio(Playback::Stream &stream, int expected);
};

void TestStream::initTestCase() {
  qRegisterMetaType<StreamMetaData>("StreamMetaData");
}

QByteArray TestStream::drain(Playback::Stream &stream) {
  QByteArray out;
  while (stream.bytesAvailable() > 0) {
    out.append(stream.read(stream.bytesAvailable()));
  }
  return out;
}

QByteArray TestStream::readAudio(Playback::Stream &stream, int expected) {
  QByteArray out;
  QElapsedTimer elapsed;
  elapsed.start();
  while (out.size() < expected && elapsed.elapsed() < kWaitMs) {
    out.append(drain(stream));
    if (out.size() >= expected) {
      break;
    }
    QTest::qWait(10);
  }
  return out;
}

void TestStream::setUrl_fillsInTheImplicitPort() {
  Playback::Stream stream(16);

  stream.setUrl(QUrl("http://radio.example/live"));
  QCOMPARE(stream.url().port(), 80);

  stream.setUrl(QUrl("https://radio.example/live"));
  QCOMPARE(stream.url().port(), 443);

  stream.setUrl(QUrl("http://radio.example:8000/live"));
  QCOMPARE(stream.url().port(), 8000);

  stream.setUrl(QUrl());
  QVERIFY(stream.url().isEmpty());
}

void TestStream::isValidUrl_acceptsHttpSchemesOnly() {
  Playback::Stream stream(16);

  stream.setUrl(QUrl("http://radio.example/live"));
  QVERIFY(stream.isValidUrl());

  stream.setUrl(QUrl("https://radio.example/live"));
  QVERIFY(stream.isValidUrl());

  stream.setUrl(QUrl("file:///music/a.mp3"));
  QVERIFY(!stream.isValidUrl());
}

void TestStream::start_rejectsANonHttpUrl() {
  Playback::Stream stream(16);
  stream.setUrl(QUrl("file:///music/a.mp3"));
  QSignalSpy failed(&stream, &Playback::Stream::error);

  QVERIFY(!stream.start());
  QCOMPARE(failed.count(), 1);
  QVERIFY(!stream.isRunning());
}

void TestStream::deliversTheBodyAndTracksTotalBytes() {
  Server server;
  QVERIFY(server.listen(QHostAddress::LocalHost));
  const QByteArray body(4096, 'a');
  server.chunks = {response("Content-Type: audio/mpeg\r\n", body)};

  Playback::Stream stream(1024);
  stream.setUrl(server.url());
  QSignalSpy started(&stream, &Playback::Stream::started);
  QVERIFY(stream.start());

  QCOMPARE(readAudio(stream, body.size()), body);
  QCOMPARE(started.count(), 1);
  QCOMPARE(stream.pos(), qint64(body.size()));
  QVERIFY(server.request.startsWith("GET /stream HTTP/1.0"));
  QVERIFY(server.request.contains("Icy-Metadata: 1"));
  stream.stop();
}

void TestStream::headerNamesAreLowerCasedIntoMetadata() {
  Server server;
  QVERIFY(server.listen(QHostAddress::LocalHost));
  // Title-Cased the way a Go/Caddy proxy rewrites them.
  server.chunks = {response("Content-Type: audio/mpeg\r\nIcy-Br: 192\r\n", QByteArray(64, 'a'))};

  Playback::Stream stream(16);
  stream.setUrl(server.url());
  QSignalSpy meta(&stream, &Playback::Stream::metadataChanged);
  QVERIFY(stream.start());

  QTRY_VERIFY_WITH_TIMEOUT(!meta.isEmpty(), kWaitMs);
  const auto last = meta.last().first().value<StreamMetaData>();
  QCOMPARE(last.bitrate(), quint16(192));
  QCOMPARE(last.format(), QString("audio/mpeg"));
  stream.stop();
}

void TestStream::icyMetadataIsStrippedFromTheAudio() {
  Server server;
  QVERIFY(server.listen(QHostAddress::LocalHost));
  const QByteArray audio(32, 'a');
  QByteArray body = audio.left(16) + icyBlock("Artist - Song") + audio.mid(16);
  server.chunks = {response("Icy-MetaInt: 16\r\n", body)};

  Playback::Stream stream(8);
  stream.setUrl(server.url());
  QSignalSpy meta(&stream, &Playback::Stream::metadataChanged);
  QVERIFY(stream.start());

  QCOMPARE(readAudio(stream, audio.size()), audio);
  QTRY_VERIFY_WITH_TIMEOUT(!meta.isEmpty(), kWaitMs);
  const auto last = meta.last().first().value<StreamMetaData>();
  QCOMPARE(last.artist(), QString("Artist"));
  QCOMPARE(last.title(), QString("Song"));
  stream.stop();
}

void TestStream::icyBlockSplitAcrossReadsIsReassembled() {
  Server server;
  QVERIFY(server.listen(QHostAddress::LocalHost));
  const QByteArray audio(32, 'a');
  const QByteArray block = icyBlock("Split - Title");
  QByteArray body = audio.left(16) + block + audio.mid(16);
  // Cut through the middle of the metadata block.
  const int cut = 16 + 1 + block.size() / 2;
  server.chunks = {response("Icy-MetaInt: 16\r\n", body.left(cut)), body.mid(cut)};

  Playback::Stream stream(8);
  stream.setUrl(server.url());
  QSignalSpy meta(&stream, &Playback::Stream::metadataChanged);
  QVERIFY(stream.start());

  QCOMPARE(readAudio(stream, audio.size()), audio);
  QTRY_VERIFY_WITH_TIMEOUT(!meta.isEmpty(), kWaitMs);
  QCOMPARE(meta.last().first().value<StreamMetaData>().title(), QString("Title"));
  stream.stop();
}

void TestStream::chunkedBodyIsDecoded() {
  Server server;
  QVERIFY(server.listen(QHostAddress::LocalHost));
  server.chunks = {response("Transfer-Encoding: chunked\r\n",
                            "4\r\nabcd\r\n3;ext=1\r\nefg\r\n0\r\n\r\n")};

  Playback::Stream stream(4);
  stream.setUrl(server.url());
  QVERIFY(stream.start());

  QCOMPARE(readAudio(stream, 7), QByteArray("abcdefg"));
  stream.stop();
}

void TestStream::chunkedHeaderSplitAcrossReadsIsDecoded() {
  Server server;
  QVERIFY(server.listen(QHostAddress::LocalHost));
  const QByteArray body = "4\r\nabcd\r\n4\r\nefgh\r\n0\r\n\r\n";
  server.chunks = {response("Transfer-Encoding: chunked\r\n", body.left(11)), body.mid(11)};

  Playback::Stream stream(4);
  stream.setUrl(server.url());
  QVERIFY(stream.start());

  QCOMPARE(readAudio(stream, 8), QByteArray("abcdefgh"));
  stream.stop();
}

void TestStream::stopClearsTheUrlAndReportsStopped() {
  Server server;
  QVERIFY(server.listen(QHostAddress::LocalHost));
  server.chunks = {response("Content-Type: audio/mpeg\r\n", QByteArray(64, 'a'))};

  Playback::Stream stream(16);
  stream.setUrl(server.url());
  QSignalSpy stopping(&stream, &Playback::Stream::stopping);
  QSignalSpy stopped(&stream, &Playback::Stream::stopped);
  QVERIFY(stream.start());
  QTRY_VERIFY_WITH_TIMEOUT(stream.bytesAvailable() > 0, kWaitMs);

  stream.stop();

  QVERIFY(stopped.count() == 1 || stopped.wait(kWaitMs));
  QVERIFY(!stream.isRunning());
  // The worker clears the url on the way out.
  QVERIFY(stream.url().isEmpty());
  QVERIFY(stopping.count() <= 1);
}

void TestStream::stopRightAfterStartDoesNotWaitForTheTimeout() {
  Server server;
  QVERIFY(server.listen(QHostAddress::LocalHost));
  // Accepts and then says nothing, so only stop() can end the worker.
  server.chunks = {};

  Playback::Stream stream(16);
  stream.setUrl(server.url());
  QSignalSpy stopped(&stream, &Playback::Stream::stopped);

  QElapsedTimer elapsed;
  elapsed.start();
  QVERIFY(stream.start());
  // The worker has almost certainly not wired up stopping() yet.
  stream.stop();

  QVERIFY(elapsed.elapsed() < 5000);
  QVERIFY(!stream.isRunning());
  QVERIFY(stopped.count() == 1 || stopped.wait(kWaitMs));
}

QTEST_GUILESS_MAIN(TestStream)
#include "tst_stream.moc"
