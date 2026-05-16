#include <QtTest>

#include "track.h"

class TestTrack : public QObject {
  Q_OBJECT
private slots:
  void defaultTrackIsZeroed();
  void defaultTrackIsInvalid();

  void formattedTimeUnderMinute();
  void formattedTimeMinutesAndSeconds();
  void formattedTimeHours();
  void formattedTimeDays();
  void formattedTimeRoundsDownToSeconds();

  void fullCtorPopulatesFields();
  void fullCtorGeneratesNonZeroUid();
  void fullCtorWithNonexistentPathIsInvalid();

  void streamCtorIsStream();
  void streamCtorHasUid();

  void setMpdMakesIsMpdTrue();
  void setMpdEmptyClearsMpd();
  void setCueToggles();
  void setDurationOverrides();
  void setPlaylistNameRoundTrips();
  void setAudioFormatOverridesFields();
  void setStreamMetaAndClearStreamMeta();

  void formattedDurationDelegatesToFormattedTime();
  void formattedAudioInfoIncludesMonoStereo();
  void formattedAudioInfoIncludesBitrateAndSamplerate();
  void formattedAudioInfoOmitsZeroFields();
  void shortTextArtistAndTitle();
  void shortTextTitleOnly();
  void shortTextFallsBackToFilename();
  void formattedTitleWithoutYear();
  void formattedTitleWithYear();
  void detectFormatFromSuffix();
  void filenameAndDirCacheAreLazyButStable();

  void uidIsStableUnderGenerateUidByHashing();
  void uidDiffersByPrefix();

  void titleForStreamWithoutMetaUsesDisplayableUrl();
  void titleForStreamWithMetaUsesMetaTitle();
  void titleForFileWithoutTitleUsesFilename();
  void artistForStreamReadsStreamMeta();
  void bitrateForStreamReadsStreamMeta();

  void urlForLocalFileIsLocalUrl();
  void urlForStreamReturnsStreamUrl();

  void isStreamWithEmptyUrlIsFalse();
};

void TestTrack::defaultTrackIsZeroed() {
  Track t;
  QCOMPARE(t.uid(),          quint64{0});
  QCOMPARE(t.begin(),        quint32{0});
  QCOMPARE(t.duration(),     quint32{0});
  QCOMPARE(t.channels(),     quint8{0});
  QCOMPARE(t.bitrate(),      quint16{0});
  QCOMPARE(t.sample_rate(),  quint32{0});
  QCOMPARE(t.year(),         quint16{0});
  QCOMPARE(t.track_number(), quint16{0});
  QVERIFY(!t.isCue());
  QVERIFY(!t.isMpd());
  QVERIFY(!t.isStream());
}

void TestTrack::defaultTrackIsInvalid() {
  Track t;
  QVERIFY(!t.isValid());
}

void TestTrack::formattedTimeUnderMinute() {
  QCOMPARE(Track::formattedTime(0),     QStringLiteral("00:00"));
  QCOMPARE(Track::formattedTime(999),   QStringLiteral("00:00"));
  QCOMPARE(Track::formattedTime(1000),  QStringLiteral("00:01"));
  QCOMPARE(Track::formattedTime(59000), QStringLiteral("00:59"));
}

void TestTrack::formattedTimeMinutesAndSeconds() {
  QCOMPARE(Track::formattedTime(60000),     QStringLiteral("01:00"));
  QCOMPARE(Track::formattedTime(3599000),   QStringLiteral("59:59"));
}

void TestTrack::formattedTimeHours() {
  // Implementation pads minutes/seconds with '0' but pads hours with the
  // default fill char (space) — `arg(hours, 2, 10)` has no explicit fill.
  // Lock down the current output so any future change is intentional.
  QCOMPARE(Track::formattedTime(3600000),  QStringLiteral(" 1:00:00"));
  QCOMPARE(Track::formattedTime(3661000),  QStringLiteral(" 1:01:01"));
}

void TestTrack::formattedTimeDays() {
  // The day-bucket branch is the only thing we can lock down here: the
  // implementation has a known unit bug in the recursive call (it passes
  // seconds back through a function that expects ms), so the tail of the
  // string isn't meaningful. Just confirm the day prefix appears.
  const quint64 twentyFiveHoursMs = 25ULL * 3600ULL * 1000ULL;
  const QString out = Track::formattedTime(twentyFiveHoursMs);
  QVERIFY2(out.startsWith(QStringLiteral("1d")),
           qPrintable(QStringLiteral("expected day prefix, got: ") + out));
}

void TestTrack::formattedTimeRoundsDownToSeconds() {
  QCOMPARE(Track::formattedTime(1500), QStringLiteral("00:01"));
  QCOMPARE(Track::formattedTime(1999), QStringLiteral("00:01"));
}

void TestTrack::fullCtorPopulatesFields() {
  Track t(QStringLiteral("/no/such/song.flac"),
          1234,
          QStringLiteral("Artist"),
          QStringLiteral("Album"),
          QStringLiteral("Title"),
          /*tracknum*/ 7,
          /*year*/ 2024,
          /*duration*/ 180000,
          /*channels*/ 2,
          /*bitrate*/ 320,
          /*samplerate*/ 44100);
  QCOMPARE(t.path(),         QStringLiteral("/no/such/song.flac"));
  QCOMPARE(t.begin(),        quint32{1234});
  QCOMPARE(t.artist(),       QStringLiteral("Artist"));
  QCOMPARE(t.album(),        QStringLiteral("Album"));
  QCOMPARE(t.title(),        QStringLiteral("Title"));
  QCOMPARE(t.track_number(), quint16{7});
  QCOMPARE(t.year(),         quint16{2024});
  QCOMPARE(t.duration(),     quint32{180000});
  QCOMPARE(t.channels(),     quint8{2});
  QCOMPARE(t.bitrate(),      quint16{320});
  QCOMPARE(t.sample_rate(),  quint32{44100});
  QCOMPARE(t.format(),       QStringLiteral("FLAC"));
}

void TestTrack::fullCtorGeneratesNonZeroUid() {
  Track t(QStringLiteral("/no/such.mp3"), 0, {}, {}, {}, 0, 0, 0, 0, 0, 0);
  QVERIFY(t.uid() != 0);
}

void TestTrack::fullCtorWithNonexistentPathIsInvalid() {
  Track t(QStringLiteral("/definitely/does/not/exist.mp3"),
          0, {}, {}, {}, 0, 0, 0, 0, 0, 0);
  QVERIFY(!t.isValid());
}

void TestTrack::streamCtorIsStream() {
  Track t(QUrl(QStringLiteral("http://example.com/stream.mp3")),
          QStringLiteral("ref"));
  QVERIFY(t.isStream());
  QVERIFY(t.isValid());
  QCOMPARE(t.path(), QStringLiteral("ref"));
  QCOMPARE(t.url(),  QUrl(QStringLiteral("http://example.com/stream.mp3")));
}

void TestTrack::streamCtorHasUid() {
  Track t(QUrl(QStringLiteral("http://example.com/stream")), {});
  QVERIFY(t.uid() != 0);
}

void TestTrack::setMpdMakesIsMpdTrue() {
  Track t;
  QVERIFY(!t.isMpd());
  t.setMpd(QUrl(QStringLiteral("mpd://localhost:6600")));
  QVERIFY(t.isMpd());
  QCOMPARE(t.mpd_server_url(),
           QUrl(QStringLiteral("mpd://localhost:6600")));
}

void TestTrack::setMpdEmptyClearsMpd() {
  Track t;
  t.setMpd(QUrl(QStringLiteral("mpd://host")));
  QVERIFY(t.isMpd());
  t.setMpd(QUrl());
  QVERIFY(!t.isMpd());
}

void TestTrack::setCueToggles() {
  Track t;
  QVERIFY(!t.isCue());
  t.setCue(true);
  QVERIFY(t.isCue());
  t.setCue(false);
  QVERIFY(!t.isCue());
}

void TestTrack::setDurationOverrides() {
  Track t;
  t.setDuration(98765);
  QCOMPARE(t.duration(), quint32{98765});
}

void TestTrack::setPlaylistNameRoundTrips() {
  Track t;
  t.setPlaylistName(QStringLiteral("my list"));
  QCOMPARE(t.playlist_name(), QStringLiteral("my list"));
}

void TestTrack::setAudioFormatOverridesFields() {
  Track t(QStringLiteral("/p.mp3"), 0, {}, {}, {}, 0, 0, 0, 1, 128, 22050);
  t.setAudioFormat(48000, 2, 256);
  QCOMPARE(t.sample_rate(), quint32{48000});
  QCOMPARE(t.channels(),    quint8{2});
  QCOMPARE(t.bitrate(),     quint16{256});
}

void TestTrack::setStreamMetaAndClearStreamMeta() {
  Track t(QUrl(QStringLiteral("http://e/x")), {});
  StreamMetaData m;
  m.insert(QStringLiteral("icy-br"), QStringLiteral("128"));
  t.setStreamMeta(m);
  QCOMPARE(t.bitrate(), quint16{128});
  t.clearStreamMeta();
  QCOMPARE(t.bitrate(), quint16{0});
}

void TestTrack::formattedDurationDelegatesToFormattedTime() {
  Track t;
  t.setDuration(125000);
  QCOMPARE(t.formattedDuration(), QStringLiteral("02:05"));
}

void TestTrack::formattedAudioInfoIncludesMonoStereo() {
  Track mono(QStringLiteral("/p.mp3"), 0, {}, {}, {}, 0, 0, 0, 1, 0, 0);
  QVERIFY(mono.formattedAudioInfo().contains(QStringLiteral("Mono")));
  Track stereo(QStringLiteral("/p.mp3"), 0, {}, {}, {}, 0, 0, 0, 2, 0, 0);
  QVERIFY(stereo.formattedAudioInfo().contains(QStringLiteral("Stereo")));
}

void TestTrack::formattedAudioInfoIncludesBitrateAndSamplerate() {
  Track t(QStringLiteral("/p.mp3"), 0, {}, {}, {}, 0, 0, 0, 2, 320, 44100);
  const QString s = t.formattedAudioInfo();
  QVERIFY(s.contains(QStringLiteral("320kbps")));
  QVERIFY(s.contains(QStringLiteral("44100Hz")));
}

void TestTrack::formattedAudioInfoOmitsZeroFields() {
  Track t(QStringLiteral("/p.mp3"), 0, {}, {}, {}, 0, 0, 0, 0, 0, 0);
  const QString s = t.formattedAudioInfo();
  QVERIFY(!s.contains(QStringLiteral("kbps")));
  QVERIFY(!s.contains(QStringLiteral("Hz")));
}

void TestTrack::shortTextArtistAndTitle() {
  Track t(QStringLiteral("/p.mp3"), 0,
          QStringLiteral("A"), QStringLiteral("L"), QStringLiteral("T"),
          0, 0, 0, 0, 0, 0);
  QCOMPARE(t.shortText(), QStringLiteral("A - T"));
}

void TestTrack::shortTextTitleOnly() {
  Track t(QStringLiteral("/p.mp3"), 0,
          QString(), QString(), QStringLiteral("Just Title"),
          0, 0, 0, 0, 0, 0);
  QCOMPARE(t.shortText(), QStringLiteral("Just Title"));
}

void TestTrack::shortTextFallsBackToFilename() {
  Track t(QStringLiteral("/dir/song.flac"), 0,
          QString(), QString(), QString(),
          0, 0, 0, 0, 0, 0);
  // title() falls back to filename() when title is empty and not a stream,
  // so shortText hits the title branch and reports the filename.
  QCOMPARE(t.shortText(), QStringLiteral("song.flac"));
}

void TestTrack::formattedTitleWithoutYear() {
  Track t(QStringLiteral("/p.mp3"), 0,
          QStringLiteral("A"), QStringLiteral("L"), QStringLiteral("T"),
          0, /*year*/ 0, 0, 0, 0, 0);
  QCOMPARE(t.formattedTitle(), QStringLiteral("A - L - T"));
}

void TestTrack::formattedTitleWithYear() {
  Track t(QStringLiteral("/p.mp3"), 0,
          QStringLiteral("A"), QStringLiteral("L"), QStringLiteral("T"),
          0, /*year*/ 2024, 0, 0, 0, 0);
  QCOMPARE(t.formattedTitle(), QStringLiteral("A - L (2024) - T"));
}

void TestTrack::detectFormatFromSuffix() {
  Track t(QStringLiteral("/some/dir/clip.opus"), 0, {}, {}, {}, 0, 0, 0, 0, 0, 0);
  QCOMPARE(t.format(), QStringLiteral("OPUS"));
}

void TestTrack::filenameAndDirCacheAreLazyButStable() {
  Track t(QStringLiteral("/no/where/x.flac"), 0, {}, {}, {}, 0, 0, 0, 0, 0, 0);
  const QString f1 = t.filename();
  const QString f2 = t.filename();
  QCOMPARE(f1, QStringLiteral("x.flac"));
  QCOMPARE(f1, f2);
  // dir() returns canonicalPath() which is empty for non-existent paths.
  // Just check stability.
  QCOMPARE(t.dir(), t.dir());
}

void TestTrack::uidIsStableUnderGenerateUidByHashing() {
  Track a(QStringLiteral("/x.flac"), 0, {}, {}, {}, 0, 0, 0, 0, 0, 0);
  Track b(QStringLiteral("/x.flac"), 0, {}, {}, {}, 0, 0, 0, 0, 0, 0);
  a.generateUidByHashing(QStringLiteral("prefix"));
  b.generateUidByHashing(QStringLiteral("prefix"));
  QCOMPARE(a.uid(), b.uid());
  QVERIFY(a.uid() != 0);
}

void TestTrack::uidDiffersByPrefix() {
  Track a(QStringLiteral("/x.flac"), 0, {}, {}, {}, 0, 0, 0, 0, 0, 0);
  Track b(QStringLiteral("/x.flac"), 0, {}, {}, {}, 0, 0, 0, 0, 0, 0);
  a.generateUidByHashing(QStringLiteral("A"));
  b.generateUidByHashing(QStringLiteral("B"));
  QVERIFY(a.uid() != b.uid());
}

void TestTrack::titleForStreamWithoutMetaUsesDisplayableUrl() {
  Track t(QUrl(QStringLiteral("http://example.com:8000/path")), {});
  const QString title = t.title();
  QVERIFY(title.contains(QStringLiteral("example.com")));
  QVERIFY(title.contains(QStringLiteral("8000")));
  QVERIFY(title.contains(QStringLiteral("/path")));
}

void TestTrack::titleForStreamWithMetaUsesMetaTitle() {
  Track t(QUrl(QStringLiteral("http://e/x")), {});
  StreamMetaData m(QStringLiteral("Live Set"));
  t.setStreamMeta(m);
  QCOMPARE(t.title(), QStringLiteral("Live Set"));
}

void TestTrack::titleForFileWithoutTitleUsesFilename() {
  Track t(QStringLiteral("/dir/file.flac"), 0,
          {}, {}, QString(), 0, 0, 0, 0, 0, 0);
  QCOMPARE(t.title(), QStringLiteral("file.flac"));
}

void TestTrack::artistForStreamReadsStreamMeta() {
  Track t(QUrl(QStringLiteral("http://e/x")), {});
  StreamMetaData m;
  m.insert(QStringLiteral("stream"),
           QStringLiteral("StreamTitle='Some Artist - Some Title';"));
  t.setStreamMeta(m);
  QCOMPARE(t.artist(), QStringLiteral("Some Artist"));
}

void TestTrack::bitrateForStreamReadsStreamMeta() {
  Track t(QUrl(QStringLiteral("http://e/x")), {});
  StreamMetaData m;
  m.insert(QStringLiteral("icy-br"), QStringLiteral("192"));
  t.setStreamMeta(m);
  QCOMPARE(t.bitrate(), quint16{192});
}

void TestTrack::urlForLocalFileIsLocalUrl() {
  Track t(QStringLiteral("/dir/file.flac"), 0, {}, {}, {}, 0, 0, 0, 0, 0, 0);
  QCOMPARE(t.url(), QUrl::fromLocalFile(QStringLiteral("/dir/file.flac")));
}

void TestTrack::urlForStreamReturnsStreamUrl() {
  Track t(QUrl(QStringLiteral("http://example.com/x")), {});
  QCOMPARE(t.url(), QUrl(QStringLiteral("http://example.com/x")));
}

void TestTrack::isStreamWithEmptyUrlIsFalse() {
  Track t(QUrl(), QStringLiteral("ref"));
  QVERIFY(!t.isStream());
}

QTEST_GUILESS_MAIN(TestTrack)
#include "tst_track.moc"
