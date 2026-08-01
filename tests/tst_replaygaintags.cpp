#include <QtTest>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include <fileref.h>
#include <mp4file.h>
#include <mp4tag.h>
#include <opusfile.h>
#include <tag.h>
#include <tpropertymap.h>
#include <xiphcomment.h>

#include "replaygain/tags.h"

class TestReplayGainTags : public QObject {
  Q_OBJECT
private slots:
  void init();

  void parsesGainWithDbSuffix();
  void parsesLowercaseAndMixedCaseKeys();
  void rejectsGarbageValues();
  void rejectsNonPositivePeak();
  void r128WinsOverReplayGainInOpus();
  void r128IgnoredForNonOpus();
  void emptyPropertiesYieldInvalidGain();

  void roundTripFlac();
  void roundTripMp3();
  void roundTripMp4();
  void roundTripOpus();
  void mp4AtomIsLowercase();
  void opusStoresR128AndNoReplayGainGain();
  void rewriteReplacesPreviousValues();
  void trackOnlyWriteClearsAlbumTags();
  void missingFileFailsToOpen();

private:
  QString fixture(const QString &name);
  static ReplayGain::Gain sampleGain();
  static void verifyRoundTrip(const QString &path);

  std::unique_ptr<QTemporaryDir> dir;
};

void TestReplayGainTags::init() {
  dir = std::make_unique<QTemporaryDir>();
  dir->setAutoRemove(true);
  QVERIFY(dir->isValid());
}

QString TestReplayGainTags::fixture(const QString &name) {
  const QString src = QString("%1/%2").arg(QStringLiteral(RG_FIXTURES_DIR), name);
  const QString dst = dir->filePath(name);
  if (!QFile::copy(src, dst)) {
    return QString();
  }
  QFile(dst).setPermissions(QFile::ReadOwner | QFile::WriteOwner);
  return dst;
}

ReplayGain::Gain TestReplayGainTags::sampleGain() {
  ReplayGain::Gain g;
  g.track_db = -5.57;
  g.track_peak = 0.910858;
  g.album_db = -2.48;
  g.album_peak = 0.977203;
  g.has_track = true;
  g.has_album = true;
  return g;
}

void TestReplayGainTags::verifyRoundTrip(const QString &path) {
  QVERIFY(!path.isEmpty());
  QCOMPARE(ReplayGain::writeTags(path, sampleGain()), ReplayGain::TagResult::Ok);

  const ReplayGain::Gain g = ReplayGain::readTags(path);
  QVERIFY(g.isValid());
  QVERIFY(g.has_track);
  QVERIFY(g.has_album);
  QVERIFY2(std::fabs(g.track_db - -5.57) < 0.01, qPrintable(QString::number(g.track_db)));
  QVERIFY2(std::fabs(g.album_db - -2.48) < 0.01, qPrintable(QString::number(g.album_db)));
  QVERIFY2(std::fabs(g.track_peak - 0.910858) < 1e-6, qPrintable(QString::number(g.track_peak)));
  QVERIFY2(std::fabs(g.album_peak - 0.977203) < 1e-6, qPrintable(QString::number(g.album_peak)));
}

void TestReplayGainTags::parsesGainWithDbSuffix() {
  TagLib::PropertyMap props;
  props.insert("REPLAYGAIN_TRACK_GAIN", TagLib::StringList("-5.57 dB"));
  props.insert("REPLAYGAIN_TRACK_PEAK", TagLib::StringList("0.910858"));
  props.insert("REPLAYGAIN_ALBUM_GAIN", TagLib::StringList("+2.48 DB"));
  props.insert("REPLAYGAIN_ALBUM_PEAK", TagLib::StringList("0.977203"));

  const ReplayGain::Gain g = ReplayGain::fromProperties(props, false);
  QVERIFY(g.has_track);
  QVERIFY(g.has_album);
  QVERIFY(std::fabs(g.track_db - -5.57) < 1e-9);
  QVERIFY(std::fabs(g.album_db - 2.48) < 1e-9);
  QVERIFY(std::fabs(g.track_peak - 0.910858) < 1e-9);
}

void TestReplayGainTags::parsesLowercaseAndMixedCaseKeys() {
  TagLib::PropertyMap props;
  props.insert("replaygain_track_gain", TagLib::StringList("-7.25 dB"));
  props.insert("ReplayGain_Album_Gain", TagLib::StringList("-1.00 dB"));

  const ReplayGain::Gain g = ReplayGain::fromProperties(props, false);
  QVERIFY(g.has_track);
  QVERIFY(g.has_album);
  QVERIFY(std::fabs(g.track_db - -7.25) < 1e-9);
  QVERIFY(std::fabs(g.album_db - -1.0) < 1e-9);
}

void TestReplayGainTags::rejectsGarbageValues() {
  TagLib::PropertyMap props;
  props.insert("REPLAYGAIN_TRACK_GAIN", TagLib::StringList("loud dB"));
  props.insert("REPLAYGAIN_ALBUM_GAIN", TagLib::StringList(""));

  const ReplayGain::Gain g = ReplayGain::fromProperties(props, false);
  QVERIFY(!g.has_track);
  QVERIFY(!g.has_album);
  QVERIFY(!g.isValid());
}

void TestReplayGainTags::rejectsNonPositivePeak() {
  TagLib::PropertyMap props;
  props.insert("REPLAYGAIN_TRACK_GAIN", TagLib::StringList("-3.00 dB"));
  props.insert("REPLAYGAIN_TRACK_PEAK", TagLib::StringList("0"));

  const ReplayGain::Gain g = ReplayGain::fromProperties(props, false);
  QVERIFY(g.has_track);
  QCOMPARE(g.track_peak, 0.0);
}

void TestReplayGainTags::r128WinsOverReplayGainInOpus() {
  TagLib::PropertyMap props;
  props.insert("R128_TRACK_GAIN", TagLib::StringList("-1792"));
  props.insert("REPLAYGAIN_TRACK_GAIN", TagLib::StringList("-99.00 dB"));

  const ReplayGain::Gain g = ReplayGain::fromProperties(props, true);
  QVERIFY(g.has_track);
  // -1792 / 256 = -7 dB against -23 LUFS, which is -2 dB against -18 LUFS.
  QVERIFY2(std::fabs(g.track_db - -2.0) < 1e-9, qPrintable(QString::number(g.track_db)));
}

void TestReplayGainTags::r128IgnoredForNonOpus() {
  TagLib::PropertyMap props;
  props.insert("R128_TRACK_GAIN", TagLib::StringList("-1792"));

  const ReplayGain::Gain g = ReplayGain::fromProperties(props, false);
  QVERIFY(!g.has_track);
}

void TestReplayGainTags::emptyPropertiesYieldInvalidGain() {
  const TagLib::PropertyMap props;
  QVERIFY(!ReplayGain::fromProperties(props, false).isValid());
  QVERIFY(!ReplayGain::fromProperties(props, true).isValid());
}

void TestReplayGainTags::roundTripFlac() {
  verifyRoundTrip(fixture(QStringLiteral("silence.flac")));
}

void TestReplayGainTags::roundTripMp3() {
  verifyRoundTrip(fixture(QStringLiteral("silence.mp3")));
}

void TestReplayGainTags::roundTripMp4() {
  verifyRoundTrip(fixture(QStringLiteral("silence.m4a")));
}

void TestReplayGainTags::roundTripOpus() {
  verifyRoundTrip(fixture(QStringLiteral("silence.opus")));
}

void TestReplayGainTags::mp4AtomIsLowercase() {
  const QString path = fixture(QStringLiteral("silence.m4a"));
  QCOMPARE(ReplayGain::writeTags(path, sampleGain()), ReplayGain::TagResult::Ok);

  TagLib::MP4::File f(QFile::encodeName(path).constData());
  QVERIFY(f.isValid());
  const auto &items = f.tag()->itemMap();
  QVERIFY(items.contains("----:com.apple.iTunes:replaygain_track_gain"));
  QVERIFY(!items.contains("----:com.apple.iTunes:REPLAYGAIN_TRACK_GAIN"));
}

void TestReplayGainTags::opusStoresR128AndNoReplayGainGain() {
  const QString path = fixture(QStringLiteral("silence.opus"));
  ReplayGain::Gain g = sampleGain();
  g.track_db = -2.0;
  QCOMPARE(ReplayGain::writeTags(path, g), ReplayGain::TagResult::Ok);

  TagLib::Ogg::Opus::File f(QFile::encodeName(path).constData());
  QVERIFY(f.isValid());
  const TagLib::PropertyMap props = f.tag()->properties();
  QVERIFY(props.contains("R128_TRACK_GAIN"));
  QVERIFY(!props.contains("REPLAYGAIN_TRACK_GAIN"));
  QVERIFY(props.contains("REPLAYGAIN_TRACK_PEAK"));
  QCOMPARE(props["R128_TRACK_GAIN"].front().toInt(), -1792);
}

void TestReplayGainTags::rewriteReplacesPreviousValues() {
  const QString path = fixture(QStringLiteral("silence.flac"));
  QCOMPARE(ReplayGain::writeTags(path, sampleGain()), ReplayGain::TagResult::Ok);

  ReplayGain::Gain second = sampleGain();
  second.track_db = -11.11;
  QCOMPARE(ReplayGain::writeTags(path, second), ReplayGain::TagResult::Ok);

  const ReplayGain::Gain g = ReplayGain::readTags(path);
  QVERIFY(std::fabs(g.track_db - -11.11) < 0.01);

  TagLib::FileRef f(QFile::encodeName(path).constData());
  QCOMPARE(f.tag()->properties()["REPLAYGAIN_TRACK_GAIN"].size(), 1U);
}

void TestReplayGainTags::trackOnlyWriteClearsAlbumTags() {
  const QString path = fixture(QStringLiteral("silence.flac"));
  QCOMPARE(ReplayGain::writeTags(path, sampleGain()), ReplayGain::TagResult::Ok);
  QVERIFY(ReplayGain::readTags(path).has_album);

  ReplayGain::Gain track_only;
  track_only.track_db = -4.0;
  track_only.track_peak = 0.5;
  track_only.has_track = true;
  QCOMPARE(ReplayGain::writeTags(path, track_only), ReplayGain::TagResult::Ok);

  const ReplayGain::Gain g = ReplayGain::readTags(path);
  QVERIFY(g.has_track);
  QVERIFY(!g.has_album);
}

void TestReplayGainTags::missingFileFailsToOpen() {
  const QString path = dir->filePath(QStringLiteral("nope.flac"));
  QCOMPARE(ReplayGain::writeTags(path, sampleGain()), ReplayGain::TagResult::OpenFailed);
  QVERIFY(!ReplayGain::readTags(path).isValid());
}

QTEST_GUILESS_MAIN(TestReplayGainTags)
#include "tst_replaygaintags.moc"
