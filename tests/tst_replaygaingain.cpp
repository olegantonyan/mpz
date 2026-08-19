#include <QtTest>

#include <cmath>

#include "replaygain/gain.h"

using ReplayGain::Gain;
using ReplayGain::Mode;
using ReplayGain::Settings;

class TestReplayGainGain : public QObject {
  Q_OBJECT
private slots:
  void offAlwaysYieldsUnityGain();
  void trackModeUsesTrackGain();
  void albumModeUsesAlbumGain();
  void albumModeFallsBackToTrack();
  void trackModeFallsBackToAlbum();
  void preampIsAdded();
  void clipPreventionCapsAtThePeak();
  void clipPreventionOffLetsItThrough();
  void unscannedTracksUseTheFallback();
  void resultIsClamped();
  void dynamicsIsOffByDefault();
  void dynamicsBoostsDenseMasters();
  void dynamicsTrimsDynamicMaterial();
  void dynamicsIsClamped();
  void dynamicsNeedsAPeak();
  void dynamicsTrimSurvivesTheClipCap();
  void dynamicsUsesAlbumValuesInAlbumMode();

private:
  static Gain trackOnly(double db, double peak);
  static Gain albumOnly(double db, double peak);
};

Gain TestReplayGainGain::trackOnly(double db, double peak) {
  Gain g;
  g.track_db = db;
  g.track_peak = peak;
  g.has_track = true;
  return g;
}

Gain TestReplayGainGain::albumOnly(double db, double peak) {
  Gain g;
  g.album_db = db;
  g.album_peak = peak;
  g.has_album = true;
  return g;
}

void TestReplayGainGain::offAlwaysYieldsUnityGain() {
  Settings s;
  s.mode = Mode::Off;
  s.preamp_db = 6.0;
  s.fallback_db = -9.0;
  QCOMPARE(ReplayGain::effectiveGainDb(trackOnly(-5.0, 0.5), s), 0.0);
  QCOMPARE(ReplayGain::effectiveGainDb(Gain(), s), 0.0);
}

void TestReplayGainGain::trackModeUsesTrackGain() {
  Settings s;
  s.mode = Mode::Track;
  s.prevent_clipping = false;

  Gain g = trackOnly(-5.0, 0.5);
  g.album_db = -12.0;
  g.album_peak = 0.9;
  g.has_album = true;

  QCOMPARE(ReplayGain::effectiveGainDb(g, s), -5.0);
}

void TestReplayGainGain::albumModeUsesAlbumGain() {
  Settings s;
  s.mode = Mode::Album;
  s.prevent_clipping = false;

  Gain g = trackOnly(-5.0, 0.5);
  g.album_db = -12.0;
  g.album_peak = 0.9;
  g.has_album = true;

  QCOMPARE(ReplayGain::effectiveGainDb(g, s), -12.0);
}

void TestReplayGainGain::albumModeFallsBackToTrack() {
  Settings s;
  s.mode = Mode::Album;
  s.prevent_clipping = false;
  QCOMPARE(ReplayGain::effectiveGainDb(trackOnly(-4.5, 0.5), s), -4.5);
}

void TestReplayGainGain::trackModeFallsBackToAlbum() {
  Settings s;
  s.mode = Mode::Track;
  s.prevent_clipping = false;
  QCOMPARE(ReplayGain::effectiveGainDb(albumOnly(-2.5, 0.5), s), -2.5);
}

void TestReplayGainGain::preampIsAdded() {
  Settings s;
  s.mode = Mode::Track;
  s.preamp_db = 3.0;
  s.prevent_clipping = false;
  QCOMPARE(ReplayGain::effectiveGainDb(trackOnly(-5.0, 0.5), s), -2.0);
}

void TestReplayGainGain::clipPreventionCapsAtThePeak() {
  Settings s;
  s.mode = Mode::Track;
  s.prevent_clipping = true;

  const double capped = ReplayGain::effectiveGainDb(trackOnly(12.0, 0.5), s);
  QVERIFY2(std::fabs(capped - 6.0206) < 0.001, qPrintable(QString::number(capped)));

  QCOMPARE(ReplayGain::effectiveGainDb(trackOnly(6.0, 1.0), s), 0.0);

  QCOMPARE(ReplayGain::effectiveGainDb(trackOnly(-8.0, 0.5), s), -8.0);
}

void TestReplayGainGain::clipPreventionOffLetsItThrough() {
  Settings s;
  s.mode = Mode::Track;
  s.prevent_clipping = false;
  QCOMPARE(ReplayGain::effectiveGainDb(trackOnly(9.0, 1.0), s), 9.0);
}

void TestReplayGainGain::unscannedTracksUseTheFallback() {
  Settings s;
  s.mode = Mode::Track;
  s.fallback_db = -6.0;
  s.preamp_db = 3.0;

  QCOMPARE(ReplayGain::effectiveGainDb(Gain(), s), -6.0);
}

void TestReplayGainGain::resultIsClamped() {
  Settings s;
  s.mode = Mode::Track;
  s.prevent_clipping = false;
  QCOMPARE(ReplayGain::effectiveGainDb(trackOnly(-99.0, 0.5), s), ReplayGain::kMinGainDb);
  QCOMPARE(ReplayGain::effectiveGainDb(trackOnly(99.0, 0.5), s), ReplayGain::kMaxGainDb);
}

// 10^(x/20): a peak that many dB from full scale
static double peakAtDb(double db) {
  return std::pow(10.0, db / 20.0);
}

void TestReplayGainGain::dynamicsIsOffByDefault() {
  Settings s;
  s.mode = Mode::Track;
  s.prevent_clipping = false;
  QCOMPARE(s.dynamics_pct, 0);
  QCOMPARE(ReplayGain::effectiveGainDb(trackOnly(-10.0, peakAtDb(1.0)), s), -10.0);
}

void TestReplayGainGain::dynamicsBoostsDenseMasters() {
  Settings s;
  s.mode = Mode::Track;
  s.prevent_clipping = false;
  s.dynamics_pct = 40;

  // -8 LUFS at +1 dBTP: PLR 9, four below the reference
  const Gain g = trackOnly(-10.0, peakAtDb(1.0));
  QVERIFY2(std::fabs(ReplayGain::plrDb(g.track_db, g.track_peak) - 9.0) < 1e-9,
           qPrintable(QString::number(ReplayGain::plrDb(g.track_db, g.track_peak))));

  const double db = ReplayGain::effectiveGainDb(g, s);
  QVERIFY2(std::fabs(db - (-8.8)) < 0.001, qPrintable(QString::number(db)));
}

void TestReplayGainGain::dynamicsTrimsDynamicMaterial() {
  Settings s;
  s.mode = Mode::Track;
  s.prevent_clipping = false;
  s.dynamics_pct = 40;

  // -25 LUFS at -1 dBTP: PLR 24
  const double db = ReplayGain::effectiveGainDb(trackOnly(7.0, peakAtDb(-1.0)), s);
  QVERIFY2(std::fabs(db - 3.7) < 0.001, qPrintable(QString::number(db)));
}

void TestReplayGainGain::dynamicsIsClamped() {
  Settings s;
  s.mode = Mode::Track;
  s.prevent_clipping = false;
  s.dynamics_pct = 100;

  // the raw correction would be -8.25
  const double db = ReplayGain::effectiveGainDb(trackOnly(7.0, peakAtDb(-1.0)), s);
  QVERIFY2(std::fabs(db - (7.0 - ReplayGain::kMaxDynamicsDb)) < 0.001,
           qPrintable(QString::number(db)));
}

void TestReplayGainGain::dynamicsNeedsAPeak() {
  Settings s;
  s.mode = Mode::Track;
  s.dynamics_pct = 100;
  QCOMPARE(ReplayGain::effectiveGainDb(trackOnly(-10.0, 0.0), s), -10.0);
}

void TestReplayGainGain::dynamicsTrimSurvivesTheClipCap() {
  Settings s;
  s.mode = Mode::Track;
  s.prevent_clipping = true;

  const Gain g = trackOnly(7.0, peakAtDb(-1.0));
  QCOMPARE(ReplayGain::effectiveGainDb(g, s), 1.0);

  s.dynamics_pct = 40;
  const double db = ReplayGain::effectiveGainDb(g, s);
  QVERIFY2(std::fabs(db - (1.0 - 3.3)) < 0.001, qPrintable(QString::number(db)));
}

void TestReplayGainGain::dynamicsUsesAlbumValuesInAlbumMode() {
  Settings s;
  s.mode = Mode::Album;
  s.prevent_clipping = false;
  s.dynamics_pct = 40;

  Gain g = albumOnly(7.0, peakAtDb(-1.0));
  g.track_db = -10.0;
  g.track_peak = peakAtDb(1.0);
  g.has_track = true;

  const double db = ReplayGain::effectiveGainDb(g, s);
  QVERIFY2(std::fabs(db - 3.7) < 0.001, qPrintable(QString::number(db)));
}

QTEST_GUILESS_MAIN(TestReplayGainGain)
#include "tst_replaygaingain.moc"
