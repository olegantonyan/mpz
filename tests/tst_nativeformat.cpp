#include <QtTest>
#include <QAudioFormat>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "decode/nativeformat.h"

class TestNativeFormat : public QObject {
  Q_OBJECT
private slots:
  void flacYieldsStreamFormat();
  void mp3YieldsInvalidFormat();
  void missingFileYieldsInvalidFormat();
  void uppercaseSuffixIsRecognized();

private:
  static QString fixture(const QString &name) {
    return QDir(QStringLiteral(AUDIO_FIXTURES_DIR)).filePath(name);
  }
};

void TestNativeFormat::flacYieldsStreamFormat() {
  const QAudioFormat format = Decode::nativeAudioFormat(fixture("silence.flac"));
  QVERIFY(format.isValid());
  QCOMPARE(format.sampleRate(), 44100);
  QCOMPARE(format.channelCount(), 2);
  QCOMPARE(format.sampleFormat(), QAudioFormat::Int16);
}

void TestNativeFormat::mp3YieldsInvalidFormat() {
  QVERIFY(!Decode::nativeAudioFormat(fixture("silence.mp3")).isValid());
}

void TestNativeFormat::missingFileYieldsInvalidFormat() {
  QVERIFY(!Decode::nativeAudioFormat(fixture("nope.flac")).isValid());
}

void TestNativeFormat::uppercaseSuffixIsRecognized() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString upper = QDir(dir.path()).filePath("silence.FLAC");
  QVERIFY(QFile::copy(fixture("silence.flac"), upper));
  QVERIFY(Decode::nativeAudioFormat(upper).isValid());
}

QTEST_GUILESS_MAIN(TestNativeFormat)
#include "tst_nativeformat.moc"
