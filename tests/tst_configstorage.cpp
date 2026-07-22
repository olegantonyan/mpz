#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>

#include "config/storage.h"

class TestConfigStorage : public QObject {
  Q_OBJECT
private slots:
  void init();          // per-test
  void cleanup();       // per-test

  void configPathRespectsEnvOverride();
  void roundTripInteger();
  void roundTripString();
  void roundTripBoolean();
  void roundTripList();
  void roundTripByteArray();
  void roundTripNestedMap();
  void getMissingKeyReportsNotOk();
  void removeDeletesAndMarksChanged();
  void persistsAcrossReopen();
  void saveWritesAppVersion();
  void castScalarParsesBooleanAndInteger();

private:
  QTemporaryDir tempDir;
};

void TestConfigStorage::init() {
  tempDir.setAutoRemove(true);
  QVERIFY(tempDir.isValid());
  qputenv("MPZ_CONFIG_DIR_OVERRIDE", tempDir.path().toUtf8());
}

void TestConfigStorage::cleanup() {
  qunsetenv("MPZ_CONFIG_DIR_OVERRIDE");
}

void TestConfigStorage::configPathRespectsEnvOverride() {
  QCOMPARE(Config::Storage::configPath(), tempDir.path());
}

void TestConfigStorage::roundTripInteger() {
  {
    Config::Storage s(QStringLiteral("a.yml"));
    s.set(QStringLiteral("port"), Config::Value(6600));
    QVERIFY(s.save());
  }
  Config::Storage s(QStringLiteral("a.yml"));
  bool ok = false;
  auto v = s.get(QStringLiteral("port"), &ok);
  QVERIFY(ok);
  QCOMPARE(v.type(),     Config::Value::Integer);
  QCOMPARE(v.get<int>(), 6600);
}

void TestConfigStorage::roundTripString() {
  {
    Config::Storage s(QStringLiteral("b.yml"));
    s.set(QStringLiteral("name"), Config::Value(QStringLiteral("zaphod")));
    QVERIFY(s.save());
  }
  Config::Storage s(QStringLiteral("b.yml"));
  bool ok = false;
  auto v = s.get(QStringLiteral("name"), &ok);
  QVERIFY(ok);
  QCOMPARE(v.type(),         Config::Value::String);
  QCOMPARE(v.get<QString>(), QStringLiteral("zaphod"));
}

void TestConfigStorage::roundTripBoolean() {
  {
    Config::Storage s(QStringLiteral("c.yml"));
    s.set(QStringLiteral("enabled"), Config::Value(true));
    QVERIFY(s.save());
  }
  Config::Storage s(QStringLiteral("c.yml"));
  bool ok = false;
  auto v = s.get(QStringLiteral("enabled"), &ok);
  QVERIFY(ok);
  QCOMPARE(v.type(),      Config::Value::Boolean);
  QCOMPARE(v.get<bool>(), true);
}

void TestConfigStorage::roundTripList() {
  {
    Config::Storage s(QStringLiteral("d.yml"));
    QList<int> sizes{ 100, 200, 300 };
    QVERIFY(s.set(QStringLiteral("splitter"), sizes));
    QVERIFY(s.save());
  }
  Config::Storage s(QStringLiteral("d.yml"));
  bool ok = false;
  auto list = s.getIntList(QStringLiteral("splitter"), &ok);
  QVERIFY(ok);
  QCOMPARE(list, (QList<int>{ 100, 200, 300 }));
}

void TestConfigStorage::roundTripByteArray() {
  const QByteArray payload = QByteArrayLiteral("\x01\x02\xff\x7f hello");
  {
    Config::Storage s(QStringLiteral("e.yml"));
    QVERIFY(s.set(QStringLiteral("blob"), payload));
    QVERIFY(s.save());
  }
  Config::Storage s(QStringLiteral("e.yml"));
  bool ok = false;
  const QByteArray out = s.getByteArray(QStringLiteral("blob"), &ok);
  QVERIFY(ok);
  QCOMPARE(out, payload);
}

void TestConfigStorage::roundTripNestedMap() {
  {
    QMap<QString, Config::Value> entry;
    entry.insert(QStringLiteral("enabled"), Config::Value(true));
    entry.insert(QStringLiteral("profile"), Config::Value(QStringLiteral("HD650")));
    QMap<QString, Config::Value> devices;
    devices.insert(QStringLiteral("default"), Config::Value(entry));

    Config::Storage s(QStringLiteral("n.yml"));
    QVERIFY(s.set(QStringLiteral("eq_devices"), Config::Value(devices)));
    QVERIFY(s.save());
  }
  Config::Storage s(QStringLiteral("n.yml"));
  bool ok = false;
  auto v = s.get(QStringLiteral("eq_devices"), &ok);
  QVERIFY(ok);
  QCOMPARE(v.type(), Config::Value::Map);
  auto entry = v.get<QMap<QString, Config::Value>>()
                 .value(QStringLiteral("default"))
                 .get<QMap<QString, Config::Value>>();
  QCOMPARE(entry.value(QStringLiteral("enabled")).get<bool>(), true);
  QCOMPARE(entry.value(QStringLiteral("profile")).get<QString>(), QStringLiteral("HD650"));
}

void TestConfigStorage::getMissingKeyReportsNotOk() {
  Config::Storage s(QStringLiteral("f.yml"));
  bool ok = true;
  auto v = s.get(QStringLiteral("nope"), &ok);
  QVERIFY(!ok);
  QVERIFY(v.isNull());
}

void TestConfigStorage::removeDeletesAndMarksChanged() {
  Config::Storage s(QStringLiteral("g.yml"));
  s.set(QStringLiteral("k"), Config::Value(1));
  QVERIFY(s.save());
  s.remove(QStringLiteral("k"));
  bool ok = true;
  s.get(QStringLiteral("k"), &ok);
  QVERIFY(!ok);
}

void TestConfigStorage::persistsAcrossReopen() {
  // Write three keys, close, reopen, verify all three.
  {
    Config::Storage s(QStringLiteral("multi.yml"));
    s.set(QStringLiteral("a"), Config::Value(1));
    s.set(QStringLiteral("b"), Config::Value(QStringLiteral("two")));
    s.set(QStringLiteral("c"), Config::Value(true));
    QVERIFY(s.save());
  }
  Config::Storage s(QStringLiteral("multi.yml"));
  QCOMPARE(s.get(QStringLiteral("a")).get<int>(),     1);
  QCOMPARE(s.get(QStringLiteral("b")).get<QString>(), QStringLiteral("two"));
  QCOMPARE(s.get(QStringLiteral("c")).get<bool>(),    true);
}

void TestConfigStorage::saveWritesAppVersion() {
  {
    Config::Storage s(QStringLiteral("ver.yml"));
    s.set(QStringLiteral("dummy"), Config::Value(1));
    QVERIFY(s.save());
  }
  Config::Storage s(QStringLiteral("ver.yml"));
  QVERIFY(!s.appVersion().isNull());
  QCOMPARE(s.appVersion(), QVersionNumber::fromString(QString(VERSION)));
}

void TestConfigStorage::castScalarParsesBooleanAndInteger() {
  // Round-trip via a raw YAML file: this exercises the parse() → castScalar()
  // path that turns "true"/"42"/plain text into typed Config::Values.
  const QString path = tempDir.filePath(QStringLiteral("raw.yml"));
  {
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    f.write("flag_true: true\n"
            "flag_no: no\n"
            "count: 42\n"
            "name: hello\n");
  }
  Config::Storage s(QStringLiteral("raw.yml"));
  QCOMPARE(s.get(QStringLiteral("flag_true")).type(), Config::Value::Boolean);
  QCOMPARE(s.get(QStringLiteral("flag_true")).get<bool>(), true);
  QCOMPARE(s.get(QStringLiteral("flag_no")).type(),   Config::Value::Boolean);
  QCOMPARE(s.get(QStringLiteral("flag_no")).get<bool>(), false);
  QCOMPARE(s.get(QStringLiteral("count")).type(),     Config::Value::Integer);
  QCOMPARE(s.get(QStringLiteral("count")).get<int>(),    42);
  QCOMPARE(s.get(QStringLiteral("name")).type(),      Config::Value::String);
  QCOMPARE(s.get(QStringLiteral("name")).get<QString>(), QStringLiteral("hello"));
}

QTEST_GUILESS_MAIN(TestConfigStorage)
#include "tst_configstorage.moc"
