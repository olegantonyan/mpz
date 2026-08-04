#include <QtTest>

#include "playlist_ui/floatingheader.h"

using PlaylistUi::FloatingHeader;

class TestFloatingHeader : public QObject {
  Q_OBJECT
private slots:
  void stripItselfIsInTheZone();
  void belowTheStripIsOutOfTheZone();
  void visibleZoneIsWiderThanHiddenZone();
  void negativeYIsOutOfTheZone();
};

void TestFloatingHeader::stripItselfIsInTheZone() {
  QVERIFY(FloatingHeader::inRevealZone(0, 22, false));
  QVERIFY(FloatingHeader::inRevealZone(22, 22, false));
}

void TestFloatingHeader::belowTheStripIsOutOfTheZone() {
  QVERIFY(!FloatingHeader::inRevealZone(200, 22, false));
  QVERIFY(!FloatingHeader::inRevealZone(200, 22, true));
}

void TestFloatingHeader::visibleZoneIsWiderThanHiddenZone() {
  QVERIFY(!FloatingHeader::inRevealZone(30, 22, false));
  QVERIFY(FloatingHeader::inRevealZone(30, 22, true));
}

void TestFloatingHeader::negativeYIsOutOfTheZone() {
  QVERIFY(!FloatingHeader::inRevealZone(-1, 22, false));
  QVERIFY(!FloatingHeader::inRevealZone(-1, 22, true));
}

QTEST_GUILESS_MAIN(TestFloatingHeader)
#include "tst_floatingheader.moc"
