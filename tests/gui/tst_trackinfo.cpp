#include "fixture.h"

#include "coverart/covers.h"
#include "lyrics/lyricswidget.h"
#include "modusoperandi.h"
#include "slidingbanner.h"
#include "playlist_ui/trackinfodialog.h"

#include <QLabel>
#include <QPlainTextEdit>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QTableView>
#include <QToolButton>

// No lyrics providers are configured, so every lyrics path here stays offline.
class TestTrackInfo : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();
  void emptyTabsAreRemovedNotHidden();
  void generalRowsDifferPerTrackKind();
  void mpdPathHasItsPasswordMasked();
  void editTagsIsHiddenForCueMpdAndStreams();
  void lyricsPreferEmbeddedOverSidecar();
  void lyricsFallBackToTheSidecarFile();
  void lyricsWidgetReportsNothingPlayingWhenCleared();
  void lyricsWidgetGivesUpWithoutProviders();

private:
  GuiTest::ConfigDir config;
  QTemporaryDir dir;
  SlidingBanner banner;
  std::unique_ptr<Config::Global> global;
  std::unique_ptr<Config::Local> local;
  std::unique_ptr<ModusOperandi> modus;
  int counter = 0;

  QString copyFixture();
  static QStringList tabTitles(TrackInfoDialog &dlg);
  static QStringList generalLabels(TrackInfoDialog &dlg);
  static Track streamTrack();
};

void TestTrackInfo::initTestCase() {
  QVERIFY(config.init());
  QVERIFY(dir.isValid());
  global = std::make_unique<Config::Global>();
  QVERIFY(global->lyricsProviders().isEmpty());
  // Track::artCover() goes through the process-wide singleton, which qFatals
  // if it was never initialised.
  local = std::make_unique<Config::Local>();
  modus = std::make_unique<ModusOperandi>(*local, &banner);
  CoverArt::Covers::instance(*modus);
}

void TestTrackInfo::init() {
  counter++;
}

QString TestTrackInfo::copyFixture() {
  const QString target = dir.filePath(QString("t%1.flac").arg(counter));
  QFile::remove(target);
  QFile::copy(QStringLiteral(AUDIO_FIXTURES_DIR) + "/silence.flac", target);
  QFile::setPermissions(target, QFile::ReadOwner | QFile::WriteOwner);
  return target;
}

Track TestTrackInfo::streamTrack() {
  return Track(QUrl("http://radio.example/live"), "radio://x", "Station");
}

QStringList TestTrackInfo::tabTitles(TrackInfoDialog &dlg) {
  auto *tabs = dlg.findChild<QTabWidget *>(QStringLiteral("tabWidget"));
  QStringList result;
  for (int i = 0; i < tabs->count(); i++) {
    result << tabs->tabText(i);
  }
  return result;
}

QStringList TestTrackInfo::generalLabels(TrackInfoDialog &dlg) {
  auto *view = dlg.findChild<QTableView *>(QStringLiteral("tableView"));
  QStringList result;
  for (int i = 0; i < view->model()->rowCount(); i++) {
    result << view->model()->index(i, 0).data().toString();
  }
  return result;
}

void TestTrackInfo::emptyTabsAreRemovedNotHidden() {
  TrackInfoDialog local_file(Track(copyFixture()), *global);
  QVERIFY(tabTitles(local_file).size() > 1);

  TrackInfoDialog stream(streamTrack(), *global);
  QCOMPARE(tabTitles(stream).size(), 1);
  QCOMPARE(tabTitles(stream).first(), tabTitles(local_file).first());
}

void TestTrackInfo::generalRowsDifferPerTrackKind() {
  TrackInfoDialog stream(streamTrack(), *global);
  const QStringList stream_rows = generalLabels(stream);
  QVERIFY(stream_rows.contains("Stream url"));
  QVERIFY(!stream_rows.contains("Track number"));
  QVERIFY(!stream_rows.contains("Duration"));

  Track cue(copyFixture(), 0, "artist", "album", "title", 1, 2000, 1000, 2, 320, 44100);
  cue.setCue();
  TrackInfoDialog cue_info(cue, *global);
  const QStringList cue_rows = generalLabels(cue_info);
  QVERIFY(cue_rows.contains("CUE start at"));
  QVERIFY(cue_rows.contains("Duration"));
  QVERIFY(!cue_rows.contains("Stream url"));
}

void TestTrackInfo::mpdPathHasItsPasswordMasked() {
  Track track("music/song.mp3", 0, "artist", "album", "title", 1, 2000, 1000, 2, 320, 44100);
  track.setMpd(QUrl("mpd://user:secret@host:6600"));

  TrackInfoDialog dlg(track, *global);
  auto *view = dlg.findChild<QTableView *>(QStringLiteral("tableView"));
  QString path;
  for (int i = 0; i < view->model()->rowCount(); i++) {
    if (view->model()->index(i, 0).data().toString() == "File path") {
      path = view->model()->index(i, 1).data().toString();
    }
  }

  QVERIFY(!path.isEmpty());
  QVERIFY2(!path.contains("secret"), qPrintable(path));
  QVERIFY(path.contains("***"));
}

void TestTrackInfo::editTagsIsHiddenForCueMpdAndStreams() {
  TrackInfoDialog local_file(Track(copyFixture()), *global);
  QVERIFY(!local_file.findChild<QToolButton *>(QStringLiteral("toolButtonEditTags"))->isHidden());

  TrackInfoDialog stream(streamTrack(), *global);
  QVERIFY(stream.findChild<QToolButton *>(QStringLiteral("toolButtonEditTags"))->isHidden());

  Track cue(copyFixture());
  cue.setCue();
  TrackInfoDialog cue_info(cue, *global);
  QVERIFY(cue_info.findChild<QToolButton *>(QStringLiteral("toolButtonEditTags"))->isHidden());
}

void TestTrackInfo::lyricsPreferEmbeddedOverSidecar() {
  const QString path = copyFixture();
  QVERIFY(GuiTest::writeLyricsTag(path, "embedded line one\nembedded line two"));
  QFile sidecar(QFileInfo(path).path() + "/" + QFileInfo(path).completeBaseName() + ".lrc");
  QVERIFY(sidecar.open(QIODevice::WriteOnly));
  sidecar.write("sidecar line\n");
  sidecar.close();

  Lyrics::Widget widget(*global);
  widget.setTrack(Track(path, 0, "artist", "album", "title", 1, 2000, 1000, 2, 320, 44100));

  QCOMPARE(widget.findChild<QLabel *>()->text(), QString("(embedded)"));
  QVERIFY(widget.findChild<QPlainTextEdit *>()->toPlainText().contains("embedded line one"));
}

void TestTrackInfo::lyricsFallBackToTheSidecarFile() {
  const QString path = copyFixture();
  QFile sidecar(QFileInfo(path).path() + "/" + QFileInfo(path).completeBaseName() + ".lrc");
  QVERIFY(sidecar.open(QIODevice::WriteOnly));
  sidecar.write("[00:01.00]timed line\nplain line\n");
  sidecar.close();

  Lyrics::Widget widget(*global);
  widget.setTrack(Track(path, 0, "artist", "album", "title", 1, 2000, 1000, 2, 320, 44100));

  QCOMPARE(widget.findChild<QLabel *>()->text(), QString("(sidecar)"));
  const QString shown = widget.findChild<QPlainTextEdit *>()->toPlainText();
  QVERIFY(shown.contains("timed line"));
  QVERIFY(!shown.contains("[00:01.00]"));
}

void TestTrackInfo::lyricsWidgetReportsNothingPlayingWhenCleared() {
  Lyrics::Widget widget(*global);
  widget.clear();

  QVERIFY(widget.findChild<QLabel *>()->text().isEmpty());
  QCOMPARE(widget.findChild<QPlainTextEdit *>()->placeholderText(), QString("Nothing playing"));
}

void TestTrackInfo::lyricsWidgetGivesUpWithoutProviders() {
  Lyrics::Widget widget(*global);

  widget.setTrack(streamTrack());
  QCOMPARE(widget.findChild<QPlainTextEdit *>()->placeholderText(), QString("No lyrics found."));

  widget.setTrack(Track(copyFixture(), 0, "artist", "album", "title", 1, 2000, 1000, 2, 320, 44100));
  QCOMPARE(widget.findChild<QPlainTextEdit *>()->placeholderText(), QString("No lyrics found."));
}

MPZ_GUI_TEST_MAIN(TestTrackInfo)
#include "tst_trackinfo.moc"
