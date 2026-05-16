#include <QtTest>

#include "mpd_client/entity.h"

using MpdClient::Entity;

class TestMpdEntity : public QObject {
  Q_OBJECT
private slots:
  void defaultIsUnknownAndEmpty();
  void defaultIsInvalid();
  void directConstructorPopulatesFields();
  void isDirOnlyTrueForDirType();
  void modifiedAtDatetimeMatchesEpoch();
  void emptyPathIsInvalidEvenIfTyped();
};

void TestMpdEntity::defaultIsUnknownAndEmpty() {
  Entity e;
  QCOMPARE(e.type(),        Entity::ENTITY_UNKNOWN);
  QCOMPARE(e.path(),        QString());
  QCOMPARE(e.modified_at(), time_t{0});
}

void TestMpdEntity::defaultIsInvalid() {
  Entity e;
  QVERIFY(!e.isValid());
  QVERIFY(!e.isDir());
}

void TestMpdEntity::directConstructorPopulatesFields() {
  Entity dir(Entity::ENTITY_DIR,  QStringLiteral("/music/jazz"), 1700000000);
  QCOMPARE(dir.type(),        Entity::ENTITY_DIR);
  QCOMPARE(dir.path(),        QStringLiteral("/music/jazz"));
  QCOMPARE(dir.modified_at(), time_t{1700000000});
  QVERIFY(dir.isValid());
  QVERIFY(dir.isDir());

  Entity song(Entity::ENTITY_SONG,
              QStringLiteral("music/file.flac"), 1700000123);
  QVERIFY(song.isValid());
  QVERIFY(!song.isDir());

  Entity pl(Entity::ENTITY_PLAYLIST,
            QStringLiteral("Daily"), 1700000200);
  QVERIFY(pl.isValid());
  QVERIFY(!pl.isDir());
}

void TestMpdEntity::isDirOnlyTrueForDirType() {
  QVERIFY( Entity(Entity::ENTITY_DIR,      QStringLiteral("a"), 0).isDir());
  QVERIFY(!Entity(Entity::ENTITY_SONG,     QStringLiteral("a"), 0).isDir());
  QVERIFY(!Entity(Entity::ENTITY_PLAYLIST, QStringLiteral("a"), 0).isDir());
  QVERIFY(!Entity(Entity::ENTITY_UNKNOWN,  QStringLiteral("a"), 0).isDir());
}

void TestMpdEntity::modifiedAtDatetimeMatchesEpoch() {
  const time_t epoch = 1700000000;
  Entity e(Entity::ENTITY_DIR, QStringLiteral("/a"), epoch);
  QCOMPARE(e.modified_at_datetime().toSecsSinceEpoch(),
           static_cast<qint64>(epoch));
}

void TestMpdEntity::emptyPathIsInvalidEvenIfTyped() {
  Entity e(Entity::ENTITY_DIR, QString(), 0);
  QVERIFY(!e.isValid());
}

QTEST_GUILESS_MAIN(TestMpdEntity)
#include "tst_mpdentity.moc"
