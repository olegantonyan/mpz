#include <QtTest>

#include "elidingcheckbox.h"

namespace {
  const QString kLongLabel = QString("Playback follows cursor ").repeated(8);
}

class TestElidingCheckBox : public QObject {
  Q_OBJECT
private slots:
  void sizePolicyAllowsShrinking();
  void minimumIsFarBelowTheFullLabel();
  void minimumKeepsTheDefaultHeight();
  void textStaysUntruncated();
};

// QCheckBox defaults to QSizePolicy::Minimum, whose missing ShrinkFlag makes layouts
// ignore minimumSizeHint() entirely.
void TestElidingCheckBox::sizePolicyAllowsShrinking() {
  ElidingCheckBox box;

  QVERIFY(box.sizePolicy().horizontalPolicy() & QSizePolicy::ShrinkFlag);
}

void TestElidingCheckBox::minimumIsFarBelowTheFullLabel() {
  ElidingCheckBox box;
  box.setText(kLongLabel);

  QVERIFY(box.minimumSizeHint().width() < box.sizeHint().width() / 4);
}

void TestElidingCheckBox::minimumKeepsTheDefaultHeight() {
  ElidingCheckBox box;
  box.setText(kLongLabel);

  QCOMPARE(box.minimumSizeHint().height(), box.QCheckBox::minimumSizeHint().height());
}

void TestElidingCheckBox::textStaysUntruncated() {
  ElidingCheckBox box;
  box.setText(kLongLabel);
  box.resize(box.minimumSizeHint());

  QCOMPARE(box.text(), kLongLabel);
}

QTEST_MAIN(TestElidingCheckBox)
#include "tst_elidingcheckbox.moc"
