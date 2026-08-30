#ifndef MPZ_GUI_TEST_FIXTURE_H
#define MPZ_GUI_TEST_FIXTURE_H

#include "config/global.h"
#include "config/local.h"
#include "mpzapplication.h"
#include "track.h"

#include <fileref.h>
#include <tpropertymap.h>

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QStringList>
#include <QTemporaryDir>
#include <QtTest>

// MainWindow reaches for MpzApplication (tray icon, Windows taskbar), which a
// plain QTEST_MAIN QApplication would make an invalid downcast.
#define MPZ_GUI_TEST_MAIN(TestClass) \
  int main(int argc, char *argv[]) { \
    MpzApplication app(argc, argv); \
    app.setApplicationName("mpz"); \
    app.setApplicationDisplayName("mpz test"); \
    QTEST_SET_MAIN_SOURCE_PATH \
    TestClass tc; \
    return QTest::qExec(&tc, argc, argv); \
  }

namespace GuiTest {
  // Isolates config, caches and the IPC socket, and pre-seeds the settings that
  // would otherwise make a headless run grab global media keys or hit the network.
  class ConfigDir {
  public:
    bool init(const QStringList &library_paths = {}) {
      if (!dir.isValid()) {
        return false;
      }
      qputenv("MPZ_CONFIG_DIR_OVERRIDE", dir.path().toUtf8());
      QStandardPaths::setTestModeEnabled(true);

      Config::Global global;
      Config::Local local;
      global.saveDisableAutoUpdateCheck(true);
      local.saveDisableQhotkey(true);
      if (!library_paths.isEmpty()) {
        local.saveLibraryPaths(library_paths);
        local.saveCurrentLibraryPath(0);
      }
      return global.sync() && local.sync();
    }

    QString path() const { return dir.path(); }

  private:
    QTemporaryDir dir;
  };

  inline Track track(const QString &title, const QString &album = "album", const QString &artist = "artist") {
    return Track("/music/" + title + ".mp3", 0, artist, album, title, 1, 2000, 1000, 2, 320, 44100);
  }

  inline QVector<Track> tracks(const QStringList &titles) {
    QVector<Track> result;
    for (const auto &t : titles) {
      result << track(t);
    }
    return result;
  }

  inline bool writeLyricsTag(const QString &path, const QString &lyrics) {
    TagLib::FileRef f(QFile::encodeName(path).constData(), false);
    if (f.isNull() || !f.tag()) {
      return false;
    }
    TagLib::PropertyMap props = f.properties();
    props.replace("LYRICS", TagLib::String(lyrics.toUtf8().constData(), TagLib::String::UTF8));
    f.setProperties(props);
    return f.save();
  }

  // Real, tag-readable files, for the paths that scan the filesystem.
  inline bool copyAudioFixtures(const QString &into) {
    if (!QDir().mkpath(into)) {
      return false;
    }
    for (const QString &name : {"silence.mp3", "silence.flac", "silence.m4a"}) {
      if (!QFile::copy(QStringLiteral(AUDIO_FIXTURES_DIR) + "/" + name, into + "/" + name)) {
        return false;
      }
    }
    return true;
  }
}

#endif // MPZ_GUI_TEST_FIXTURE_H
