#include <QtTest>

#include "playlist_ui/columnsconfig.h"
#include "config/value.h"
#include "track.h"

using PlaylistUi::ColumnsConfig;

namespace {
  Track mk(const QString &filepath = QStringLiteral("/dir/song.flac"),
           const QString &artist   = QStringLiteral("Some Artist"),
           const QString &album    = QStringLiteral("Some Album"),
           const QString &title    = QStringLiteral("Some Title"),
           quint16 year     = 2024,
           quint64 duration = 125000,
           quint16 tracknum = 3,
           quint8  channels = 2,
           quint16 bitrate  = 320,
           quint32 samplerate = 44100) {
    return Track(filepath, 0, artist, album, title, tracknum, year, duration,
                 channels, bitrate, samplerate);
  }
}

class TestColumnsConfig : public QObject {
  Q_OBJECT
private slots:
  void defaultsHaveFiveColumns();
  void defaultWidthsAndStretches();
  void defaultFieldsAreCanonical();
  void defaultAlignsHaveVCenter();
  void valueByFieldName();
  void valueForYearZeroIsEmpty();
  void valueForLengthZeroIsEmpty();
  void valueForUnknownFieldIsEmpty();
  void settersReplaceContents();
  void serializeRoundTripsThroughDeserialize();
  void vaidateThrowsOnMismatchedVectorSizes();
  void deserializeAlignRightSetsAlignRight();
};

void TestColumnsConfig::defaultsHaveFiveColumns() {
  ColumnsConfig cfg;
  QCOMPARE(cfg.count(), 5);
}

void TestColumnsConfig::defaultWidthsAndStretches() {
  ColumnsConfig cfg;
  QCOMPARE(cfg.width(1), 0.28);
  QCOMPARE(cfg.width(2), 0.28);
  QCOMPARE(cfg.width(3), 0.28);
  QCOMPARE(cfg.width(4), 0.05);
  QCOMPARE(cfg.width(5), 0.0);
  QVERIFY(!cfg.stretch(1));
  QVERIFY(!cfg.stretch(2));
  QVERIFY(!cfg.stretch(3));
  QVERIFY(!cfg.stretch(4));
  QVERIFY( cfg.stretch(5));
}

void TestColumnsConfig::defaultFieldsAreCanonical() {
  ColumnsConfig cfg;
  QCOMPARE(cfg.field(1), QStringLiteral("artist"));
  QCOMPARE(cfg.field(2), QStringLiteral("album"));
  QCOMPARE(cfg.field(3), QStringLiteral("title"));
  QCOMPARE(cfg.field(4), QStringLiteral("year"));
  QCOMPARE(cfg.field(5), QStringLiteral("length"));
}

void TestColumnsConfig::defaultAlignsHaveVCenter() {
  ColumnsConfig cfg;
  for (int col = 1; col <= cfg.count(); ++col) {
    QVERIFY(cfg.align(col) & Qt::AlignVCenter);
  }
  QVERIFY(cfg.align(4) & Qt::AlignRight);
  QVERIFY(cfg.align(5) & Qt::AlignRight);
  QVERIFY(!(cfg.align(1) & Qt::AlignRight));
}

void TestColumnsConfig::valueByFieldName() {
  ColumnsConfig cfg;
  Track t = mk();
  QCOMPARE(cfg.value(1, t), QStringLiteral("Some Artist"));
  QCOMPARE(cfg.value(2, t), QStringLiteral("Some Album"));
  QCOMPARE(cfg.value(3, t), QStringLiteral("Some Title"));
  QCOMPARE(cfg.value(4, t), QStringLiteral("2024"));
  QCOMPARE(cfg.value(5, t), t.formattedDuration());
}

void TestColumnsConfig::valueForYearZeroIsEmpty() {
  ColumnsConfig cfg;
  Track t = mk(QStringLiteral("/p.mp3"), QStringLiteral("A"),
               QStringLiteral("L"), QStringLiteral("T"),
               /*year*/ 0);
  QCOMPARE(cfg.value(4, t), QString());
}

void TestColumnsConfig::valueForLengthZeroIsEmpty() {
  ColumnsConfig cfg;
  Track t = mk(QStringLiteral("/p.mp3"), QStringLiteral("A"),
               QStringLiteral("L"), QStringLiteral("T"),
               /*year*/ 2024, /*duration*/ 0);
  QCOMPARE(cfg.value(5, t), QString());
}

void TestColumnsConfig::valueForUnknownFieldIsEmpty() {
  ColumnsConfig cfg;
  cfg.setFields(QVector<QString>{ QStringLiteral("nonsense"),
                                  QStringLiteral("album"),
                                  QStringLiteral("title"),
                                  QStringLiteral("year"),
                                  QStringLiteral("length") });
  Track t = mk();
  QCOMPARE(cfg.value(1, t), QString());
}

void TestColumnsConfig::settersReplaceContents() {
  ColumnsConfig cfg;
  cfg.setWidths(QVector<double>{ 0.1, 0.2 });
  cfg.setStretches(QVector<bool>{ true, false });
  cfg.setFields(QVector<QString>{ QStringLiteral("path"), QStringLiteral("bitrate") });
  cfg.setAligns(QVector<Qt::Alignment>{ Qt::AlignVCenter, Qt::AlignVCenter });
  QCOMPARE(cfg.count(), 2);
  QCOMPARE(cfg.width(1), 0.1);
  QCOMPARE(cfg.stretch(1), true);
  QCOMPARE(cfg.field(2), QStringLiteral("bitrate"));
}

void TestColumnsConfig::serializeRoundTripsThroughDeserialize() {
  ColumnsConfig original;
  original.setAligns(QVector<Qt::Alignment>{
    Qt::AlignVCenter,
    Qt::AlignVCenter | Qt::AlignRight,
    Qt::AlignVCenter,
    Qt::AlignVCenter | Qt::AlignRight,
    Qt::AlignVCenter | Qt::AlignRight
  });

  Config::Value serialized = original.serialize();
  ColumnsConfig roundtripped = ColumnsConfig::deserialize(serialized);

  QCOMPARE(roundtripped.count(), original.count());
  for (int col = 1; col <= original.count(); ++col) {
    QCOMPARE(roundtripped.field(col),   original.field(col));
    QCOMPARE(roundtripped.stretch(col), original.stretch(col));
    // Widths are stored as integer percents, so within 0.01 is the contract.
    QVERIFY(qAbs(roundtripped.width(col) - original.width(col)) < 0.011);
    // Aligns are reduced to left/right + VCenter on the way through.
    const bool wantRight = (original.align(col) & Qt::AlignRight) == Qt::AlignRight;
    const bool gotRight  = (roundtripped.align(col) & Qt::AlignRight) == Qt::AlignRight;
    QCOMPARE(gotRight, wantRight);
  }
}

void TestColumnsConfig::vaidateThrowsOnMismatchedVectorSizes() {
  ColumnsConfig cfg;
  cfg.setWidths(QVector<double>{ 0.5 });  // size 1, others still size 5
  bool threw = false;
  try {
    cfg.vaidate();
  } catch (...) {
    threw = true;
  }
  QVERIFY(threw);
}

void TestColumnsConfig::deserializeAlignRightSetsAlignRight() {
  QMap<QString, Config::Value> entry;
  entry[QStringLiteral("width_percent")] = Config::Value(50);
  entry[QStringLiteral("stretch")]       = Config::Value(false);
  entry[QStringLiteral("field")]         = Config::Value(QStringLiteral("year"));
  entry[QStringLiteral("align")]         = Config::Value(QStringLiteral("right"));
  QList<Config::Value> list{ Config::Value(entry) };
  Config::Value serialized(list);

  ColumnsConfig cfg = ColumnsConfig::deserialize(serialized);
  QCOMPARE(cfg.count(), 1);
  QVERIFY(cfg.align(1) & Qt::AlignRight);
}

QTEST_GUILESS_MAIN(TestColumnsConfig)
#include "tst_columnsconfig.moc"
