#include <QtTest>

#include "dynamic_range/logformat.h"

using DynamicRange::Entry;
using DynamicRange::LogMeta;
using DynamicRange::Result;

namespace {
  Result mk(double dr, double peak_db, double rms_db) {
    Result r;
    r.valid = true;
    r.dr = dr;
    r.peak_db = peak_db;
    r.rms_db = rms_db;
    return r;
  }

  Entry mkEntry(const QString &artist, const QString &album, const QString &display,
                quint32 duration_ms, const Result &result) {
    Entry e;
    e.artist = artist;
    e.album = album;
    e.display = display;
    e.duration_ms = duration_ms;
    e.sample_rate = 44100;
    e.channels = 2;
    e.bits_per_sample = 16;
    e.bitrate = 921;
    e.codec = "FLAC";
    e.result = result;
    return e;
  }

  LogMeta mkMeta() {
    LogMeta m;
    m.app_version = "2.1.2";
    m.when = QDateTime(QDate(2026, 8, 1), QTime(14, 14, 13));
    return m;
  }

  QStringList linesOf(const QString &log) {
    return log.split('\n');
  }
}

class TestDrLogFormat : public QObject {
  Q_OBJECT
private slots:
  void goldenLog();
  void groupsNonContiguousIndexes();
  void durationsAreNotZeroPadded();
  void silentTrackPrintsInf();
  void heterogeneousSummaryIsVarious();
  void failedTrackIsExcludedFromCount();
};

void TestDrLogFormat::goldenLog() {
  QVector<Entry> entries;
  entries << mkEntry("Artist", "Album", "01-First", 248000, mk(7.6, -0.30, -9.95))
          << mkEntry("Other Artist", "Album", "02-Second", 221000, mk(6.4, -0.25, -11.42));

  const QString expected = QStringList({
    "mpz 2.1.2 / Dynamic Range Meter 1.0",
    "log date: 2026-08-01 14:14:13",
    QString(80, '-'),
    "Analyzed: Artist / Album (1)",
    "          Other Artist / Album (2)",
    QString(80, '-'),
    "DR         Peak         RMS     Duration Track",
    QString(80, '-'),
    "DR8       -0.30 dB    -9.95 dB      4:08 01-First",
    "DR6       -0.25 dB   -11.42 dB      3:41 02-Second",
    QString(80, '-'),
    "Number of tracks:  2",
    "Official DR value: DR7",
    "Samplerate:        44100 Hz",
    "Channels:          2",
    "Bits per sample:   16",
    "Bitrate:           921 kbps",
    "Codec:             FLAC",
    QString(80, '='),
  }).join('\n') + '\n';

  QCOMPARE(DynamicRange::formatLog(entries, mkMeta()), expected);
}

void TestDrLogFormat::groupsNonContiguousIndexes() {
  QVector<Entry> entries;
  entries << mkEntry("A", "X", "01", 1000, mk(6.0, -1.0, -7.0))
          << mkEntry("A", "X", "02", 1000, mk(6.0, -1.0, -7.0))
          << mkEntry("B", "Y", "03", 1000, mk(6.0, -1.0, -7.0))
          << mkEntry("A", "X", "04", 1000, mk(6.0, -1.0, -7.0));

  const QStringList lines = linesOf(DynamicRange::formatLog(entries, mkMeta()));
  QCOMPARE(lines.at(3), QString("Analyzed: A / X (1-2,4)"));
  QCOMPARE(lines.at(4), QString("          B / Y (3)"));
}

void TestDrLogFormat::durationsAreNotZeroPadded() {
  QVector<Entry> entries;
  entries << mkEntry("A", "X", "01", 248000, mk(6.0, -1.0, -7.0))
          << mkEntry("A", "X", "02", 3723000, mk(6.0, -1.0, -7.0));

  const QStringList lines = linesOf(DynamicRange::formatLog(entries, mkMeta()));
  QVERIFY(lines.at(7).endsWith("      4:08 01"));
  QVERIFY(lines.at(8).endsWith("   1:02:03 02"));
}

void TestDrLogFormat::silentTrackPrintsInf() {
  QVector<Entry> entries;
  entries << mkEntry("A", "X", "01", 1000,
                     mk(0.0, DynamicRange::MINUS_INF_DB, DynamicRange::MINUS_INF_DB));

  const QStringList lines = linesOf(DynamicRange::formatLog(entries, mkMeta()));
  QCOMPARE(lines.at(7), QString("DR0        -inf dB     -inf dB      0:01 01"));
}

void TestDrLogFormat::heterogeneousSummaryIsVarious() {
  QVector<Entry> entries;
  entries << mkEntry("A", "X", "01", 1000, mk(6.0, -1.0, -7.0))
          << mkEntry("A", "X", "02", 1000, mk(6.0, -1.0, -7.0));
  entries[1].sample_rate = 48000;
  entries[1].codec = "MP3";

  const QString log = DynamicRange::formatLog(entries, mkMeta());
  QVERIFY(log.contains("Samplerate:        various\n"));
  QVERIFY(log.contains("Codec:             various\n"));
  QVERIFY(log.contains("Channels:          2\n"));
}

void TestDrLogFormat::failedTrackIsExcludedFromCount() {
  QVector<Entry> entries;
  entries << mkEntry("A", "X", "01", 248000, mk(8.0, -0.3, -9.9))
          << mkEntry("A", "X", "02", 248000, Result());

  const QString log = DynamicRange::formatLog(entries, mkMeta());
  QVERIFY(log.contains("n/a            n/a         n/a      4:08 02\n"));
  QVERIFY(log.contains("Number of tracks:  1\n"));
  QVERIFY(log.contains("Official DR value: DR8\n"));
}

QTEST_GUILESS_MAIN(TestDrLogFormat)
#include "tst_drlogformat.moc"
