#include <QtTest>
#include <QLocalSocket>
#include <QStandardPaths>

#include "ipc/instance.h"

namespace {
  struct Reply {
    bool connected = false;
    bool dropped = false;
    QByteArray body;
  };

  // Instance::send() blocks on waitForReadyRead, which starves the server living
  // in the same process, so the client side is hand-rolled and fully async here.
  QByteArray request(const QByteArray &body) {
    return QByteArray("POST / HTTP/1.1\r\n\r\n\r\n") + body;
  }

  Reply talk(const QByteArray &payload) {
    Reply reply;
    QLocalSocket socket;
    QObject::connect(&socket, &QLocalSocket::readyRead, [&]() { reply.body.append(socket.readAll()); });

    socket.connectToServer(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/single_instance");
    reply.connected = socket.waitForConnected(1000);
    if (!reply.connected) {
      return reply;
    }
    socket.write(payload);
    for (int i = 0; i < 200 && reply.body.isEmpty() && socket.state() != QLocalSocket::UnconnectedState; i++) {
      QTest::qWait(5);
    }
    reply.dropped = reply.body.isEmpty() && socket.state() == QLocalSocket::UnconnectedState;
    return reply;
  }
}

class TestIpcInstance : public QObject {
  Q_OBJECT
private slots:
  void initTestCase();
  void send_returnsMinusOneWithoutAServer();
  void answersWithThePidAndEmitsLoadFiles();
  void answersAMalformedBodyWithoutEmitting();
  void oversizedPayloadIsDroppedUnanswered();

private:
  // Constructed only after test mode is on: Instance caches its socket name,
  // which QStandardPaths would otherwise resolve to the real cache dir.
  std::unique_ptr<IPC::Instance> server;
};

void TestIpcInstance::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  server = std::make_unique<IPC::Instance>();
  QVERIFY(server->start());
}

void TestIpcInstance::send_returnsMinusOneWithoutAServer() {
  IPC::Instance client(100);
  QCOMPARE(client.anotherPid(), -1);
}

void TestIpcInstance::answersWithThePidAndEmitsLoadFiles() {
  QSignalSpy spy(server.get(), &IPC::Instance::load_files_received);

  const Reply reply = talk(request(R"({"load_files":["/a.mp3","/b.flac"]})"));

  QVERIFY(reply.connected);
  QVERIFY(reply.body.startsWith("HTTP/1.1 257 R U OK"));
  QVERIFY(reply.body.contains(QByteArray::number(qApp->applicationPid())));
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.first().first().toStringList(), QStringList({"/a.mp3", "/b.flac"}));
}

void TestIpcInstance::answersAMalformedBodyWithoutEmitting() {
  QSignalSpy spy(server.get(), &IPC::Instance::load_files_received);

  for (const char *body : {"not json", "[1,2]", R"({"load_files":"nope"})"}) {
    const Reply reply = talk(request(body));
    QVERIFY2(reply.body.startsWith("HTTP/1.1 257 R U OK"), body);
  }
  QCOMPARE(spy.count(), 0);
}

void TestIpcInstance::oversizedPayloadIsDroppedUnanswered() {
  QSignalSpy spy(server.get(), &IPC::Instance::load_files_received);

  // No header terminator, so the server keeps buffering until the cap trips.
  const Reply reply = talk(QByteArray(64 * 1024 + 1, 'x'));

  QVERIFY(reply.connected);
  QVERIFY(reply.dropped);
  QCOMPARE(spy.count(), 0);
}

QTEST_GUILESS_MAIN(TestIpcInstance)
#include "tst_ipcinstance.moc"
