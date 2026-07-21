#include <QtTest>

#include "coverart/foldercover.h"

using CoverArt::FolderCover::best;
using CoverArt::FolderCover::score;

class TestFolderCover : public QObject {
  Q_OBJECT
private slots:
  void frontBeatsBack();
  void coverBeatsBackCover();
  void backCoverIsNegative();
  void nonFrontNamesAreNegative();
  void frontLikeNamesArePositive();
  void largeBeatsSmall();
  void genericIsNeutral();
  void caseAndSeparatorInvariant();
  void shortTokensNotPenalized();
  void emptyListYieldsEmpty();
  void tiesAreDeterministic();
};

void TestFolderCover::frontBeatsBack() {
  QCOMPARE(best({"back.jpg", "front.jpg"}).file, QString("front.jpg"));
}

void TestFolderCover::coverBeatsBackCover() {
  QCOMPARE(best({"back cover.jpg", "cover.jpg"}).file, QString("cover.jpg"));
}

void TestFolderCover::backCoverIsNegative() {
  QVERIFY(score("back cover.jpg") < 0);
  QVERIFY(score("cover_back.jpg") < 0);
}

void TestFolderCover::nonFrontNamesAreNegative() {
  QVERIFY(score("back.jpg") < 0);
  QVERIFY(score("disc.png") < 0);
  QVERIFY(score("booklet-01.jpg") < 0);
  QVERIFY(score("tray.jpg") < 0);
}

void TestFolderCover::frontLikeNamesArePositive() {
  QVERIFY(score("front.jpg") > 0);
  QVERIFY(score("cover.jpg") > 0);
  QVERIFY(score("folder.jpg") > 0);
  QVERIFY(score("AlbumArt.jpg") > 0);
  QVERIFY(score("artwork.png") > 0);
}

void TestFolderCover::largeBeatsSmall() {
  QVERIFY(score("AlbumArt_Large.jpg") > score("AlbumArtSmall.jpg"));
  QVERIFY(score("AlbumArtSmall.jpg") > 0);
  QCOMPARE(best({"AlbumArtSmall.jpg", "AlbumArt_Large.jpg"}).file,
           QString("AlbumArt_Large.jpg"));
}

void TestFolderCover::genericIsNeutral() {
  QCOMPARE(score("IMG_1234.jpg"), 0);
  QCOMPARE(score("P8290153.jpg"), 0);
}

void TestFolderCover::caseAndSeparatorInvariant() {
  QVERIFY(score("FRONT.JPG") > 0);
  QVERIFY(score("front_cover.jpg") > 0);
  QVERIFY(score("front-cover.jpg") > 0);
  QVERIFY(score("frontcover.jpg") > 0);
}

void TestFolderCover::shortTokensNotPenalized() {
  QCOMPARE(score("acdc.jpg"), 0);
  QCOMPARE(score("cd1.jpg"), 0);
}

void TestFolderCover::emptyListYieldsEmpty() {
  QVERIFY(best({}).file.isEmpty());
}

void TestFolderCover::tiesAreDeterministic() {
  QCOMPARE(best({"b.png", "a.png"}).file, QString("a.png"));
}

QTEST_GUILESS_MAIN(TestFolderCover)
#include "tst_foldercover.moc"
