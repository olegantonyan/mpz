#include <QtTest>

#include "directory_ui/radiolibrary.h"

class TestRadioLibrary : public QObject {
  Q_OBJECT
private slots:
  void isRadioLibraryPath_matchesTheSchemeOnly();
  void libraryPathLabel_namesRadio();
  void libraryPathLabel_passesLocalPathsThrough();
  void libraryPathLabel_masksTheMpdPassword();
};

void TestRadioLibrary::isRadioLibraryPath_matchesTheSchemeOnly() {
  QCOMPARE(DirectoryUi::radioLibraryPath(), QStringLiteral("radio://"));
  QVERIFY(DirectoryUi::isRadioLibraryPath("radio://"));
  QVERIFY(DirectoryUi::isRadioLibraryPath("radio://somafm-lush"));
  QVERIFY(!DirectoryUi::isRadioLibraryPath("mpd://localhost:6600"));
  QVERIFY(!DirectoryUi::isRadioLibraryPath("/home/user/music"));
  QVERIFY(!DirectoryUi::isRadioLibraryPath(""));
}

void TestRadioLibrary::libraryPathLabel_namesRadio() {
  QCOMPARE(DirectoryUi::libraryPathLabel("radio://somafm-lush"), QString("Radio"));
}

void TestRadioLibrary::libraryPathLabel_passesLocalPathsThrough() {
  QCOMPARE(DirectoryUi::libraryPathLabel("/home/user/music"), QString("/home/user/music"));
  QCOMPARE(DirectoryUi::libraryPathLabel("mpd://user@host:6600"), QString("mpd://user@host:6600"));
}

void TestRadioLibrary::libraryPathLabel_masksTheMpdPassword() {
  const QString label = DirectoryUi::libraryPathLabel("mpd://user:secret@host:6600");
  QVERIFY2(!label.contains("secret"), qPrintable(label));
  QCOMPARE(label, QString("mpd://user:***@host:6600"));
}

QTEST_GUILESS_MAIN(TestRadioLibrary)
#include "tst_radiolibrary.moc"
