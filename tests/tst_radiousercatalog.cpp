#include <QtTest>

#include "radio/catalog.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

using Radio::Catalog;

// Drives Catalog::active() over a user-supplied radio.json. The built-in
// list here comes from radiofixtures.qrc (two stations), not the shipped one.
class TestRadioUserCatalog : public QObject {
  Q_OBJECT
private slots:
  void init();
  void cleanup();

  void builtinUsedWhenNoUserFile();
  void builtinUsedWhenUserFileAbsent();
  void userFileReplacesBuiltinEntirely();
  void extendsBuiltinAppendsStations();
  void extendsBuiltinOverridesById();
  void malformedUserFileFallsBackToBuiltin();
  void malformedUserFileReportsError();
  void invalidSchemeUserFileFallsBackToBuiltin();
  void reloadPicksUpEdits();
  void clearingPathRestoresBuiltin();

private:
  void writeUser(const QString &contents);
  QString userPath() const;

  QTemporaryDir *dir = nullptr;
};

void TestRadioUserCatalog::init() {
  dir = new QTemporaryDir();
  QVERIFY(dir->isValid());
  Catalog::setUserFilePath(userPath());
}

void TestRadioUserCatalog::cleanup() {
  Catalog::setUserFilePath(QString());
  delete dir;
  dir = nullptr;
}

QString TestRadioUserCatalog::userPath() const {
  return dir->filePath(QStringLiteral("radio.json"));
}

void TestRadioUserCatalog::writeUser(const QString &contents) {
  QFile f(userPath());
  QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
  QCOMPARE(f.write(contents.toUtf8()), qint64(contents.toUtf8().size()));
  f.close();
  Catalog::reload();
}

void TestRadioUserCatalog::builtinUsedWhenNoUserFile() {
  Catalog::setUserFilePath(QString());
  const auto &c = Catalog::active();
  QCOMPARE(c.stations().size(), 2);
  QVERIFY(c.byId("builtin-one") != nullptr);
  QVERIFY(Catalog::lastError().isEmpty());
}

void TestRadioUserCatalog::builtinUsedWhenUserFileAbsent() {
  QVERIFY(!QFile::exists(userPath()));
  QCOMPARE(Catalog::active().stations().size(), 2);
  QVERIFY(Catalog::lastError().isEmpty());
}

void TestRadioUserCatalog::userFileReplacesBuiltinEntirely() {
  writeUser(R"({"stations":[{"id":"mine","name":"Mine","group":"Mine",
    "url":"https://example.net/s","codec":"mp3","bitrate":128}]})");

  const auto &c = Catalog::active();
  QCOMPARE(c.stations().size(), 1);
  QVERIFY(c.byId("mine") != nullptr);
  QCOMPARE(c.byId("builtin-one"), nullptr);
  QCOMPARE(c.byId("builtin-two"), nullptr);
  QVERIFY(Catalog::lastError().isEmpty());
}

void TestRadioUserCatalog::extendsBuiltinAppendsStations() {
  writeUser(R"({"extends_builtin":true,"stations":[{"id":"mine","name":"Mine","group":"Mine",
    "url":"https://example.net/s","codec":"mp3","bitrate":128}]})");

  const auto &c = Catalog::active();
  QCOMPARE(c.stations().size(), 3);
  QVERIFY(c.byId("builtin-one") != nullptr);
  QVERIFY(c.byId("builtin-two") != nullptr);
  QVERIFY(c.byId("mine") != nullptr);
  // The built-in stations are ungrouped, so only the user's group shows up.
  QCOMPARE(c.groups(), QStringList({"Mine"}));
}

void TestRadioUserCatalog::extendsBuiltinOverridesById() {
  writeUser(R"({"extends_builtin":true,"stations":[{"id":"builtin-one","name":"Repointed",
    "group":"Builtin","url":"https://example.net/new","codec":"aac","bitrate":320}]})");

  const auto &c = Catalog::active();
  QCOMPARE(c.stations().size(), 2);
  const auto *s = c.byId("builtin-one");
  QVERIFY(s);
  QCOMPARE(s->name, QStringLiteral("Repointed"));
  QCOMPARE(s->url, QStringLiteral("https://example.net/new"));
  QVERIFY(c.byId("builtin-two") != nullptr);
}

void TestRadioUserCatalog::malformedUserFileFallsBackToBuiltin() {
  writeUser(R"({"stations":[{"id":"mine",)");

  const auto &c = Catalog::active();
  QCOMPARE(c.stations().size(), 2);
  QVERIFY(c.byId("builtin-one") != nullptr);
}

void TestRadioUserCatalog::malformedUserFileReportsError() {
  writeUser(R"({"stations":[{"id":"mine",)");
  Catalog::active();
  QVERIFY(!Catalog::lastError().isEmpty());
}

void TestRadioUserCatalog::invalidSchemeUserFileFallsBackToBuiltin() {
  writeUser(R"({"stations":[{"id":"mine","name":"Mine","group":"Mine",
    "url":"ftp://example.net/s","codec":"mp3","bitrate":128}]})");

  QCOMPARE(Catalog::active().stations().size(), 2);
  QVERIFY(!Catalog::lastError().isEmpty());
}

void TestRadioUserCatalog::reloadPicksUpEdits() {
  writeUser(R"({"stations":[{"id":"first","name":"First","group":"G",
    "url":"https://example.net/a","codec":"mp3","bitrate":128}]})");
  QVERIFY(Catalog::active().byId("first") != nullptr);

  writeUser(R"({"stations":[{"id":"second","name":"Second","group":"G",
    "url":"https://example.net/b","codec":"mp3","bitrate":128}]})");
  QCOMPARE(Catalog::active().byId("first"), nullptr);
  QVERIFY(Catalog::active().byId("second") != nullptr);
}

void TestRadioUserCatalog::clearingPathRestoresBuiltin() {
  writeUser(R"({"stations":[{"id":"mine","name":"Mine","group":"Mine",
    "url":"https://example.net/s","codec":"mp3","bitrate":128}]})");
  QCOMPARE(Catalog::active().stations().size(), 1);

  Catalog::setUserFilePath(QString());
  QCOMPARE(Catalog::active().stations().size(), 2);
}

QTEST_MAIN(TestRadioUserCatalog)
#include "tst_radiousercatalog.moc"
