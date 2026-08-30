#include "server.h"

#include "mpd_client/client.h"

#include <QSemaphore>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <QtTest>

namespace {
  const int kDefaultMpdPort = 6600;

  // Client::probe blocks the calling thread inside libmpdclient, so a fake
  // server sharing that thread would never get to accept. It runs its own event
  // loop instead.
  class FakeMpd : public QThread {
  public:
    enum Behaviour { Answer, HangUpAfterGreeting, SilentAfterGreeting };

    explicit FakeMpd(Behaviour b) : behaviour(b) {}

    ~FakeMpd() override {
      quit();
      wait(5000);
    }

    bool listenOn(quint16 wanted) {
      requested_port = wanted;
      start();
      if (!ready.tryAcquire(1, 5000)) {
        return false;
      }
      return bound;
    }

    quint16 port() const { return bound_port; }

  protected:
    void run() override {
      QTcpServer server;
      bound = server.listen(QHostAddress::LocalHost, requested_port);
      bound_port = server.serverPort();
      ready.release();
      if (!bound) {
        return;
      }
      QObject::connect(&server, &QTcpServer::newConnection, &server, [this, &server]() {
        auto *sock = server.nextPendingConnection();
        sock->write("OK MPD 0.23.0\n");
        sock->flush();
        if (behaviour == HangUpAfterGreeting) {
          sock->disconnectFromHost();
          return;
        }
        if (behaviour == SilentAfterGreeting) {
          return;
        }
        QObject::connect(sock, &QTcpSocket::readyRead, sock, [sock]() {
          while (sock->canReadLine()) {
            const QByteArray line = sock->readLine().trimmed();
            sock->write(line == "stats" ? "songs: 7\nOK\n" : "OK\n");
            sock->flush();
          }
        });
      });
      exec();
    }

  private:
    Behaviour behaviour;
    quint16 requested_port = 0;
    quint16 bound_port = 0;
    bool bound = false;
    QSemaphore ready;
  };

  // A port nothing listens on: bind it, read it back, then let it go.
  int closedPort() {
    QTcpServer probe;
    if (!probe.listen(QHostAddress::LocalHost, 0)) {
      return 0;
    }
    const int p = probe.serverPort();
    probe.close();
    return p;
  }
}

class TestMpdConnection : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void cleanup();

  void probe_reportsTheProtocolVersionAndSongCount();
  void probe_failsOnARefusedPort();
  void probe_reportsAClosedConnectionWhenTheServerHangsUp();
  void probe_reportsATimeoutWhenTheServerNeverAnswers();
  void probe_reportsHostNotFoundForAnUnresolvableHost();
  void probe_succeedsWithTheRightPassword();
  void probe_reportsAuthenticationFailureForAWrongPassword();
  void probe_reportsPermissionDeniedWithoutAPassword();
  void probe_defaultsToTheStandardPortWhenTheUrlHasNone();

  void open_emitsConnectedAndAnswersPing();
  void open_recordsTheCurrentUrl();
  void open_authenticatesWithThePasswordInTheUrl();
  void open_emitsErrorForAWrongPassword();
  void close_emitsDisconnectedAndClearsTheUrl();
  void commandsReturnDefaultsBeforeAnyConnection();
  void reopeningEmitsDisconnectedForThePreviousServer();
  void reopeningSwitchesToTheOtherServer();
  void openingABadUrlDoesNotRetryForever();
  void readOnlyPasswordAllowsReadsAndDeniesWrites();
  void watchdogReconnectsAfterTheServerRestarts();

private:
  MpdTest::Server server;
  MpdTest::Server auth_server;
  MpdTest::Server ro_server;
  std::unique_ptr<MpdClient::Client> client;
};

void TestMpdConnection::initTestCase() {
  if (!MpdTest::Server::installed()) {
    QSKIP("mpd is not installed");
  }
  MpdTest::registerMetaTypes();

  QVERIFY2(server.start(), qPrintable(server.failReason()));

  MpdTest::Options full;
  full.password = "s3cr3t";
  QVERIFY2(auth_server.start(full), qPrintable(auth_server.failReason()));

  MpdTest::Options read_only;
  read_only.password = "ro";
  read_only.permissions = {"read"};
  QVERIFY2(ro_server.start(read_only), qPrintable(ro_server.failReason()));

  client = std::make_unique<MpdClient::Client>();
}

void TestMpdConnection::cleanup() {
  // A live connection would otherwise keep its watchdog ticking into the next case.
  client->closeConnection();
  // The client re-emits its worker's signals across threads, so they are queued
  // and only land once an event loop spins. Drain them here or a stale
  // disconnected shows up in the next case's spy.
  client->ping();
  QTest::qWait(50);
  server.resetState();
}

void TestMpdConnection::probe_reportsTheProtocolVersionAndSongCount() {
  const auto result = client->probe(server.url());
  QVERIFY2(result.first, qPrintable(result.second));
  QVERIFY(result.second.contains("protocol version"));
  QVERIFY(result.second.contains(QString("%1 songs").arg(MpdTest::Server::kSongCount)));
}

void TestMpdConnection::probe_failsOnARefusedPort() {
  const int port = closedPort();
  QVERIFY(port > 0);
  const auto result = client->probe(QUrl(QString("mpd://127.0.0.1:%1").arg(port)));
  QVERIFY(!result.first);
  QVERIFY(!result.second.isEmpty());
}

void TestMpdConnection::probe_reportsAClosedConnectionWhenTheServerHangsUp() {
  FakeMpd fake(FakeMpd::HangUpAfterGreeting);
  QVERIFY(fake.listenOn(0));

  const auto result = client->probe(QUrl(QString("mpd://127.0.0.1:%1").arg(fake.port())));
  QVERIFY(!result.first);
  QVERIFY(!result.second.isEmpty());
}

void TestMpdConnection::probe_reportsATimeoutWhenTheServerNeverAnswers() {
  FakeMpd fake(FakeMpd::SilentAfterGreeting);
  QVERIFY(fake.listenOn(0));

  const auto result = client->probe(QUrl(QString("mpd://127.0.0.1:%1").arg(fake.port())));
  QVERIFY(!result.first);
  QVERIFY(result.second.contains("timeout"));
}

void TestMpdConnection::probe_reportsHostNotFoundForAnUnresolvableHost() {
  // .invalid is reserved, so it must never resolve.
  const auto result = client->probe(QUrl("mpd://mpz-nothing-here.invalid:6600"));
  QVERIFY(!result.first);
  QVERIFY(!result.second.isEmpty());
}

void TestMpdConnection::probe_succeedsWithTheRightPassword() {
  const auto result = client->probe(auth_server.url("s3cr3t"));
  QVERIFY2(result.first, qPrintable(result.second));
  QVERIFY(result.second.contains("protocol version"));
}

void TestMpdConnection::probe_reportsAuthenticationFailureForAWrongPassword() {
  const auto result = client->probe(auth_server.url("wrong"));
  QVERIFY(!result.first);
  QVERIFY2(result.second.contains("authentication failed"), qPrintable(result.second));
}

void TestMpdConnection::probe_reportsPermissionDeniedWithoutAPassword() {
  // Configuring any password zeroes the unauthenticated default permissions, so
  // the greeting still succeeds and the first real command is refused.
  const auto result = client->probe(auth_server.url());
  QVERIFY(!result.first);
  QVERIFY2(result.second.contains("permission denied"), qPrintable(result.second));
}

void TestMpdConnection::probe_defaultsToTheStandardPortWhenTheUrlHasNone() {
  FakeMpd fake(FakeMpd::Answer);
  if (!fake.listenOn(kDefaultMpdPort)) {
    QSKIP("port 6600 is already in use");
  }

  const auto result = client->probe(QUrl("mpd://127.0.0.1"));
  QVERIFY2(result.first, qPrintable(result.second));
  QVERIFY2(result.second.contains("7 songs"), qPrintable(result.second));
}

void TestMpdConnection::open_emitsConnectedAndAnswersPing() {
  QSignalSpy connected(client.get(), &MpdClient::Client::connected);

  client->openConnection(server.url());

  // openConnection is queued and ping blocks on the same worker queue, so a
  // successful ping proves open() already ran.
  QVERIFY(client->ping());
  QTRY_COMPARE(connected.count(), 1);
  QCOMPARE(connected.first().at(0).toUrl(), server.url());
}

void TestMpdConnection::open_recordsTheCurrentUrl() {
  client->openConnection(server.url());
  QVERIFY(client->ping());

  QCOMPARE(client->currentUrl(), server.url());
}

void TestMpdConnection::open_authenticatesWithThePasswordInTheUrl() {
  client->openConnection(auth_server.url("s3cr3t"));
  QVERIFY(client->ping());

  // Reads and writes both need the password to have been accepted.
  QVERIFY(!client->lsDir("").isEmpty());
  QVERIFY(client->createPlaylist({"tagged/one.mp3"}, "authed"));
  QVERIFY(client->removePlaylist("authed"));
}

void TestMpdConnection::open_emitsErrorForAWrongPassword() {
  QSignalSpy error(client.get(), &MpdClient::Client::error);

  client->openConnection(auth_server.url("wrong"));

  QTRY_VERIFY(error.count() >= 1);
  QVERIFY(!client->ping());
  QVERIFY2(error.first().at(1).toString().contains("authentication failed"),
           qPrintable(error.first().at(1).toString()));
}

void TestMpdConnection::close_emitsDisconnectedAndClearsTheUrl() {
  client->openConnection(server.url());
  QVERIFY(client->ping());

  QSignalSpy disconnected(client.get(), &MpdClient::Client::disconnected);
  client->closeConnection();

  QTRY_COMPARE(disconnected.count(), 1);
  QCOMPARE(disconnected.first().at(0).toUrl(), server.url());
  QVERIFY(client->currentUrl().isEmpty());
  QVERIFY(!client->ping());
}

void TestMpdConnection::commandsReturnDefaultsBeforeAnyConnection() {
  MpdClient::Client fresh;

  QVERIFY(!fresh.ping());
  QVERIFY(fresh.currentUrl().isEmpty());
  QVERIFY(fresh.lsDir("").isEmpty());
  QVERIFY(fresh.playlists().isEmpty());
  QVERIFY(fresh.lsQueueSongs().isEmpty());
  QVERIFY(fresh.outputs().isEmpty());
  QVERIFY(fresh.albumArt("tagged/one.mp3").isEmpty());
  QCOMPARE(fresh.status().state, MpdClient::Status::UnknownState);
  QCOMPARE(fresh.status().volume, -1);
  QCOMPARE(fresh.currentSong().id, -1);
}

void TestMpdConnection::reopeningEmitsDisconnectedForThePreviousServer() {
  client->openConnection(server.url());
  QVERIFY(client->ping());

  QSignalSpy disconnected(client.get(), &MpdClient::Client::disconnected);
  client->openConnection(auth_server.url("s3cr3t"));
  QVERIFY(client->ping());

  // Models reset on disconnected, so replacing a live connection has to announce it.
  QTRY_COMPARE(disconnected.count(), 1);
  QCOMPARE(disconnected.first().at(0).toUrl(), server.url());
}

void TestMpdConnection::reopeningSwitchesToTheOtherServer() {
  client->openConnection(auth_server.url("s3cr3t"));
  QVERIFY(client->ping());
  QVERIFY(client->createPlaylist({"tagged/one.mp3"}, "only_on_auth"));

  client->openConnection(server.url());
  QVERIFY(client->ping());

  QCOMPARE(client->currentUrl(), server.url());
  QStringList names;
  for (const auto &entity : client->playlists()) {
    names << entity.path();
  }
  QVERIFY(!names.contains("only_on_auth"));

  client->openConnection(auth_server.url("s3cr3t"));
  QVERIFY(client->ping());
  QVERIFY(client->removePlaylist("only_on_auth"));
}

void TestMpdConnection::openingABadUrlDoesNotRetryForever() {
  const int port = closedPort();
  QVERIFY(port > 0);

  QSignalSpy error(client.get(), &MpdClient::Client::error);
  client->openConnection(QUrl(QString("mpd://127.0.0.1:%1").arg(port)));
  QTRY_VERIFY(error.count() >= 1);

  // The watchdog interval is 8.5 s. A url that never connected must not keep
  // retrying it and re-emitting error every interval.
  const int settled = error.count();
  QTest::qWait(11000);
  QCOMPARE(error.count(), settled);
}

void TestMpdConnection::readOnlyPasswordAllowsReadsAndDeniesWrites() {
  client->openConnection(ro_server.url("ro"));
  QVERIFY(client->ping());

  QVERIFY(!client->lsDir("").isEmpty());
  QVERIFY(!client->createPlaylist({"tagged/one.mp3"}, "denied"));
}

void TestMpdConnection::watchdogReconnectsAfterTheServerRestarts() {
  MpdTest::Server own;
  QVERIFY2(own.start(), qPrintable(own.failReason()));

  MpdClient::Client reconnecting;
  QSignalSpy connected(&reconnecting, &MpdClient::Client::connected);
  QSignalSpy disconnected(&reconnecting, &MpdClient::Client::disconnected);
  QSignalSpy error(&reconnecting, &MpdClient::Client::error);

  reconnecting.openConnection(own.url());
  QVERIFY(reconnecting.ping());
  QTRY_COMPARE(connected.count(), 1);

  // Restart before the watchdog fires, so one interval is enough: the ping
  // fails, the loss is announced, and the re-open finds the server already back.
  own.crash();
  QVERIFY2(own.restartSamePort(), qPrintable(own.failReason()));

  QTRY_VERIFY_WITH_TIMEOUT(disconnected.count() >= 1, 20000);
  QVERIFY(error.count() >= 1);
  QVERIFY2(error.first().at(1).toString().contains("connection lost"),
           qPrintable(error.first().at(1).toString()));

  QTRY_VERIFY_WITH_TIMEOUT(reconnecting.ping(), 30000);
  QVERIFY(connected.count() >= 2);

  reconnecting.closeConnection();
}

QTEST_GUILESS_MAIN(TestMpdConnection)

#include "tst_mpdconnection.moc"
