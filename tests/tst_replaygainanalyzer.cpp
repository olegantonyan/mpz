#include <QtTest>

#include <cmath>
#include <vector>

#include "replaygain/analyzer.h"

namespace {
  const unsigned long kRate = 48000;
  const unsigned kChannels = 2;
  const double kFreq = 997.0;
  const std::size_t kFrames = kRate * 5;

  std::vector<float> stereoSine(double amplitude, std::size_t frames = kFrames) {
    std::vector<float> pcm(frames * kChannels);
    for (std::size_t i = 0; i < frames; i++) {
      const double v = amplitude * std::sin(2.0 * M_PI * kFreq * static_cast<double>(i) / kRate);
      pcm[i * kChannels] = static_cast<float>(v);
      pcm[i * kChannels + 1] = static_cast<float>(v);
    }
    return pcm;
  }

  double measure(const std::vector<float> &pcm) {
    ReplayGain::Analyzer a(kChannels, kRate);
    if (!a.isValid() || !a.addFloat(pcm.data(), pcm.size() / kChannels)) {
      return -HUGE_VAL;
    }
    return a.integratedLufs();
  }
}

class TestReplayGainAnalyzer : public QObject {
  Q_OBJECT
private slots:
  void invalidParametersYieldInvalidAnalyzer();
  void silenceHasNoGatedLoudness();
  void loudnessTracksAmplitude();
  void int16MatchesFloat();
  void uint8MatchesFloat();
  void truePeakOfFullScaleSine();
  void albumOfIdenticalTracksEqualsOne();
  void albumIsNotTheAverageOfTrackLoudness();
  void referenceLevelConversions();
};

void TestReplayGainAnalyzer::invalidParametersYieldInvalidAnalyzer() {
  ReplayGain::Analyzer zero_channels(0, kRate);
  QVERIFY(!zero_channels.isValid());
  ReplayGain::Analyzer zero_rate(2, 0);
  QVERIFY(!zero_rate.isValid());

  const auto pcm = stereoSine(0.5, 1024);
  QVERIFY(!zero_rate.addFloat(pcm.data(), 1024));
  QVERIFY(!std::isfinite(zero_rate.integratedLufs()));
  QCOMPARE(zero_rate.truePeak(), 0.0);
}

void TestReplayGainAnalyzer::silenceHasNoGatedLoudness() {
  const std::vector<float> pcm(kFrames * kChannels, 0.0f);
  QVERIFY(!std::isfinite(measure(pcm)));
}

void TestReplayGainAnalyzer::loudnessTracksAmplitude() {
  const double loud = measure(stereoSine(0.5));
  const double quiet = measure(stereoSine(0.25));
  QVERIFY(std::isfinite(loud));
  QVERIFY(std::isfinite(quiet));
  QVERIFY2(std::fabs((loud - quiet) - 6.0206) < 0.01,
           qPrintable(QString("halving the amplitude moved loudness by %1 LU").arg(loud - quiet)));

  const double expected = -0.691 + 20.0 * std::log10(0.5);
  QVERIFY2(std::fabs(loud - expected) < 1.0,
           qPrintable(QString("measured %1 LUFS, modelled %2 LUFS").arg(loud).arg(expected)));
}

void TestReplayGainAnalyzer::int16MatchesFloat() {
  const auto pcm = stereoSine(0.5);
  std::vector<int16_t> ints(pcm.size());
  for (std::size_t i = 0; i < pcm.size(); i++) {
    ints[i] = static_cast<int16_t>(std::lround(pcm[i] * 32767.0));
  }

  ReplayGain::Analyzer a(kChannels, kRate);
  QVERIFY(a.addInt16(ints.data(), kFrames));

  const double diff = std::fabs(a.integratedLufs() - measure(pcm));
  QVERIFY2(diff < 0.05, qPrintable(QString("int16 and float differ by %1 LU").arg(diff)));
}

void TestReplayGainAnalyzer::uint8MatchesFloat() {
  const auto pcm = stereoSine(0.5);
  std::vector<uint8_t> bytes(pcm.size());
  for (std::size_t i = 0; i < pcm.size(); i++) {
    bytes[i] = static_cast<uint8_t>(std::clamp(std::lround(pcm[i] * 128.0) + 128L, 0L, 255L));
  }

  ReplayGain::Analyzer a(kChannels, kRate);
  QVERIFY(a.addUInt8(bytes.data(), kFrames));

  const double diff = std::fabs(a.integratedLufs() - measure(pcm));
  QVERIFY2(diff < 0.2, qPrintable(QString("uint8 and float differ by %1 LU").arg(diff)));
}

void TestReplayGainAnalyzer::truePeakOfFullScaleSine() {
  const auto pcm = stereoSine(1.0);
  ReplayGain::Analyzer a(kChannels, kRate);
  QVERIFY(a.addFloat(pcm.data(), kFrames));

  const double peak = a.truePeak();
  QVERIFY2(peak >= 0.99 && peak < 1.10,
           qPrintable(QString("true peak of a full-scale sine was %1").arg(peak)));
}

void TestReplayGainAnalyzer::albumOfIdenticalTracksEqualsOne() {
  const auto pcm = stereoSine(0.5);

  ReplayGain::Analyzer first(kChannels, kRate);
  ReplayGain::Analyzer second(kChannels, kRate);
  QVERIFY(first.addFloat(pcm.data(), kFrames));
  QVERIFY(second.addFloat(pcm.data(), kFrames));

  const double album = ReplayGain::Analyzer::albumLufs({&first, &second});
  QVERIFY(std::isfinite(album));
  QVERIFY2(std::fabs(album - first.integratedLufs()) < 0.01,
           qPrintable(QString("album %1 vs track %2").arg(album).arg(first.integratedLufs())));
}

void TestReplayGainAnalyzer::albumIsNotTheAverageOfTrackLoudness() {
  const auto loud_pcm = stereoSine(0.5);
  const auto quiet_pcm = stereoSine(0.05);

  ReplayGain::Analyzer loud(kChannels, kRate);
  ReplayGain::Analyzer quiet(kChannels, kRate);
  QVERIFY(loud.addFloat(loud_pcm.data(), kFrames));
  QVERIFY(quiet.addFloat(quiet_pcm.data(), kFrames));

  const double album = ReplayGain::Analyzer::albumLufs({&loud, &quiet});
  QVERIFY(std::isfinite(album));

  const double mean = (loud.integratedLufs() + quiet.integratedLufs()) / 2.0;
  QVERIFY2(std::fabs(album - mean) > 1.0,
           qPrintable(QString("album %1 collapsed onto the arithmetic mean %2").arg(album).arg(mean)));
  QVERIFY(album > mean);
  QVERIFY(album <= loud.integratedLufs() + 0.01);
}

void TestReplayGainAnalyzer::referenceLevelConversions() {
  QCOMPARE(ReplayGain::gainDbFromLufs(-18.0), 0.0);
  QVERIFY(std::fabs(ReplayGain::gainDbFromLufs(-23.0) - 5.0) < 1e-9);

  // -23 LUFS is 0 dB of R128 gain, +5 dB against ReplayGain's -18 LUFS.
  QCOMPARE(ReplayGain::r128FromLufs(-23.0), 0);
  QVERIFY(std::fabs(ReplayGain::dbFromR128(0) - 5.0) < 1e-9);

  const int q = ReplayGain::r128FromLufs(-16.0);
  QCOMPARE(q, -1792);
  QVERIFY(std::fabs(ReplayGain::dbFromR128(q) - ReplayGain::gainDbFromLufs(-16.0)) < 0.01);

  QCOMPARE(ReplayGain::r128FromLufs(-1000.0), 32767);
  QCOMPARE(ReplayGain::r128FromLufs(1000.0), -32768);
}

QTEST_GUILESS_MAIN(TestReplayGainAnalyzer)
#include "tst_replaygainanalyzer.moc"
