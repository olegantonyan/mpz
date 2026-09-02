#include <QtTest>
#include <QBuffer>
#include <QImage>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>

#include "config/local.h"
#include "coverart/covers.h"
#include "coverart/online/provider.h"
#include "modusoperandi.h"
#include "slidingbanner.h"

namespace {
  bool writeImage(const QString &path, int side) {
    QImage img(side, side, QImage::Format_RGB32);
    img.fill(Qt::red);
    return img.save(path);
  }

  QByteArray pngBytes(int side) {
    QImage img(side, side, QImage::Format_RGB32);
    img.fill(Qt::blue);
    QByteArray out;
    QBuffer buffer(&out);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, "PNG");
    return out;
  }

  // Serves one canned body, so downloadImage's acceptance rules can be driven offline.
  class Server : public QTcpServer {
  public:
    QByteArray body;
    QByteArray status = "200 OK";

    explicit Server(QObject *parent = nullptr) : QTcpServer(parent) {}

    QUrl url() const { return QUrl(QString("http://127.0.0.1:%1/cover").arg(serverPort())); }

  protected:
    void incomingConnection(qintptr handle) override {
      auto *socket = new QTcpSocket(this);
      socket->setSocketDescriptor(handle);
      connect(socket, &QTcpSocket::readyRead, socket, [this, socket]() {
        socket->readAll();
        socket->write("HTTP/1.1 " + status + "\r\nContent-Length: " +
                      QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body);
        socket->disconnectFromHost();
      });
    }
  };

  class TestProvider : public CoverArt::Online::Provider {
  public:
    void fetch(const CoverArt::Online::AlbumQuery &) override {}
    using CoverArt::Online::Provider::downloadImage;
    using CoverArt::Online::Provider::emitFailed;
    using CoverArt::Online::Provider::emitFound;
    using CoverArt::Online::Provider::emitNotFound;
    using CoverArt::Online::Provider::makeRequest;
  };
}

class TestCovers : public QObject {
  Q_OBJECT
private slots:
  void initTestCase();
  void emptyPathYieldsNothing();
  void folderCoverIsMemoizedPerDirectory();
  void embeddedArtIsNotMemoized();
  void descendsOneLevelWhenTheAlbumFolderHasNone();
  void provider_emitsExactlyOnce();
  void provider_acceptsAUsableImage();
  void provider_rejectsATinyImage();
  void provider_rejectsANonImageBody();
  void provider_reportsNotFoundOn404();
  void provider_userAgentCarriesTheContactForm();

private:
  QTemporaryDir cfg;
  SlidingBanner banner;
  std::unique_ptr<Config::Local> local;
  std::unique_ptr<ModusOperandi> modus;
  int album_counter = 0;

  QString newAlbumDir();
};

void TestCovers::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  QVERIFY(cfg.isValid());
  qputenv("MPZ_CONFIG_DIR_OVERRIDE", cfg.path().toUtf8());

  // Covers is a process-wide singleton over one ModusOperandi, so both live for the whole binary and every test gets a fresh directory instead.
  local = std::make_unique<Config::Local>();
  modus = std::make_unique<ModusOperandi>(*local, &banner);
  CoverArt::Covers::instance(*modus);
}

QString TestCovers::newAlbumDir() {
  const QString dir = cfg.path() + QString("/album%1").arg(++album_counter);
  QDir().mkpath(dir);
  return dir;
}

void TestCovers::emptyPathYieldsNothing() {
  QVERIFY(CoverArt::Covers::instance().get(QString(), "artist", "album").isEmpty());
}

void TestCovers::folderCoverIsMemoizedPerDirectory() {
  const QString dir = newAlbumDir();
  QVERIFY(writeImage(dir + "/cover.jpg", 300));

  const QString first = CoverArt::Covers::instance().get(dir + "/a.mp3", "artist", "album");
  QCOMPARE(QFileInfo(first).fileName(), QString("cover.jpg"));

  // Memoized by directory: a sibling track resolves to the same file, and replacing the image on disk does not invalidate it.
  QVERIFY(QFile::remove(dir + "/cover.jpg"));
  QCOMPARE(CoverArt::Covers::instance().get(dir + "/b.mp3", "artist", "album"), first);
}

void TestCovers::embeddedArtIsNotMemoized() {
  const QString dir = newAlbumDir();
  // No image and no tags: nothing to find, and nothing cached for the directory.
  QVERIFY(CoverArt::Covers::instance().get(dir + "/a.mp3", "artist", "album").isEmpty());

  QVERIFY(writeImage(dir + "/folder.png", 300));

  // A cover dropped in afterwards is picked up because the miss was not cached.
  QCOMPARE(QFileInfo(CoverArt::Covers::instance().get(dir + "/a.mp3", "artist", "album")).fileName(),
           QString("folder.png"));
}

void TestCovers::descendsOneLevelWhenTheAlbumFolderHasNone() {
  const QString dir = newAlbumDir();
  QVERIFY(QDir().mkpath(dir + "/scans"));
  QVERIFY(writeImage(dir + "/scans/front.jpg", 300));

  QCOMPARE(QFileInfo(CoverArt::Covers::instance().get(dir + "/a.mp3", "artist", "album")).fileName(),
           QString("front.jpg"));
}

void TestCovers::provider_emitsExactlyOnce() {
  TestProvider provider;
  QSignalSpy found(&provider, &CoverArt::Online::Provider::found);
  QSignalSpy not_found(&provider, &CoverArt::Online::Provider::notFound);
  QSignalSpy failed(&provider, &CoverArt::Online::Provider::failed);

  provider.emitFound("x", "png");
  provider.emitNotFound();
  provider.emitFailed("boom");

  QCOMPARE(found.count(), 1);
  QCOMPARE(not_found.count(), 0);
  QCOMPARE(failed.count(), 0);
}

void TestCovers::provider_acceptsAUsableImage() {
  Server server;
  QVERIFY(server.listen(QHostAddress::LocalHost));
  server.body = pngBytes(200);

  TestProvider provider;
  QSignalSpy found(&provider, &CoverArt::Online::Provider::found);
  provider.downloadImage(server.url());

  QVERIFY(found.wait(10000));
  QCOMPARE(found.first().first().toByteArray(), server.body);
  QCOMPARE(found.first().at(1).toString(), QString("png"));
}

void TestCovers::provider_rejectsATinyImage() {
  Server server;
  QVERIFY(server.listen(QHostAddress::LocalHost));
  server.body = pngBytes(10);

  TestProvider provider;
  QSignalSpy not_found(&provider, &CoverArt::Online::Provider::notFound);
  provider.downloadImage(server.url());

  QVERIFY(not_found.wait(10000));
}

void TestCovers::provider_rejectsANonImageBody() {
  Server server;
  QVERIFY(server.listen(QHostAddress::LocalHost));
  server.body = "not an image at all";

  TestProvider provider;
  QSignalSpy not_found(&provider, &CoverArt::Online::Provider::notFound);
  provider.downloadImage(server.url());

  QVERIFY(not_found.wait(10000));
}

void TestCovers::provider_reportsNotFoundOn404() {
  Server server;
  QVERIFY(server.listen(QHostAddress::LocalHost));
  server.status = "404 Not Found";

  TestProvider provider;
  QSignalSpy not_found(&provider, &CoverArt::Online::Provider::notFound);
  QSignalSpy failed(&provider, &CoverArt::Online::Provider::failed);
  provider.downloadImage(server.url());

  QVERIFY(not_found.wait(10000));
  QCOMPARE(failed.count(), 0);
}

void TestCovers::provider_userAgentCarriesTheContactForm() {
  TestProvider provider;
  const QUrl url("http://example.invalid/");

  const QString plain = provider.makeRequest(url).header(QNetworkRequest::UserAgentHeader).toString();
  const QString contact =
      provider.makeRequest(url, CoverArt::Online::Provider::Ua::Contact)
          .header(QNetworkRequest::UserAgentHeader).toString();

  QVERIFY(plain.startsWith("mpz/"));
  QVERIFY(!plain.contains("github.com"));
  QVERIFY(contact.startsWith("mpz/"));
  QVERIFY(contact.contains("github.com/olegantonyan/mpz"));
}

QTEST_MAIN(TestCovers)
#include "tst_covers.moc"
