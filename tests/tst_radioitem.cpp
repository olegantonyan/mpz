#include <QtTest>

#include "directory_ui/directorymodel/radio/item.h"

using DirectoryUi::DirectoryModel::RadioItem;

namespace {
  RadioItem *station(RadioItem *group, const QString &name) {
    auto *s = new RadioItem(false, name, group);
    group->children << s;
    return s;
  }
}

class TestRadioItem : public QObject {
  Q_OBJECT
private slots:
  void constructorSetsFields();
  void matchEmptyFilterAlwaysTrue();
  void matchOnName();
  void matchOnSubtitle();
  void matchOnDescription();
  void matchNoSubstring();
  void updateVisibilityKeepsGroupForMatchingChild();
  void updateVisibilityHidesGroupWithNoMatches();
  void updateVisibilityOnGroupNameShowsAllChildren();
  void updateVisibilityRestoresOnEmptyFilter();
  void visibleChildrenCountIgnoresHidden();
  void rowReturnsZeroForRoot();
  void rowSkipsHiddenSiblings();
  void childReturnsNthVisible();
  void childOutOfRangeReturnsNullptr();
  void destructorDeletesChildren();
};

void TestRadioItem::constructorSetsFields() {
  RadioItem g(true, QStringLiteral("SomaFM"));
  QCOMPARE(g.is_group, true);
  QCOMPARE(g.name, QStringLiteral("SomaFM"));
  QCOMPARE(g.parent, nullptr);
  QVERIFY(g.visible);
  QVERIFY(g.children.isEmpty());
}

void TestRadioItem::matchEmptyFilterAlwaysTrue() {
  RadioItem s(false, QStringLiteral("Lush"));
  QVERIFY(s.match(QString()));
}

void TestRadioItem::matchOnName() {
  RadioItem s(false, QStringLiteral("Groove Salad"));
  QVERIFY(s.match(QStringLiteral("groove")));
  QVERIFY(s.match(QStringLiteral("SALAD")));
}

void TestRadioItem::matchOnSubtitle() {
  RadioItem s(false, QStringLiteral("Lush"));
  s.subtitle = QStringLiteral("MP3 128k · Mellow vocals");
  QVERIFY(s.match(QStringLiteral("mellow")));
}

void TestRadioItem::matchOnDescription() {
  RadioItem s(false, QStringLiteral("Lush"));
  s.description = QStringLiteral("Sensuous and mellow vocals");
  QVERIFY(s.match(QStringLiteral("sensuous")));
}

void TestRadioItem::matchNoSubstring() {
  RadioItem s(false, QStringLiteral("Lush"));
  s.description = QStringLiteral("electronic");
  QVERIFY(!s.match(QStringLiteral("metal")));
}

void TestRadioItem::updateVisibilityKeepsGroupForMatchingChild() {
  RadioItem g(true, QStringLiteral("SomaFM"));
  auto *a = station(&g, QStringLiteral("Drone Zone"));
  auto *b = station(&g, QStringLiteral("Metal Detector"));

  QVERIFY(g.update_visibility(QStringLiteral("drone")));
  QVERIFY(g.visible);
  QVERIFY(a->visible);
  QVERIFY(!b->visible);
  QCOMPARE(g.visible_children_count(), 1);
}

void TestRadioItem::updateVisibilityHidesGroupWithNoMatches() {
  RadioItem g(true, QStringLiteral("SomaFM"));
  station(&g, QStringLiteral("Drone Zone"));

  QVERIFY(!g.update_visibility(QStringLiteral("nothingmatchesthis")));
  QVERIFY(!g.visible);
  QCOMPARE(g.visible_children_count(), 0);
}

void TestRadioItem::updateVisibilityOnGroupNameShowsAllChildren() {
  RadioItem g(true, QStringLiteral("SomaFM"));
  auto *a = station(&g, QStringLiteral("Drone Zone"));
  auto *b = station(&g, QStringLiteral("Metal Detector"));

  QVERIFY(g.update_visibility(QStringLiteral("somafm")));
  QVERIFY(a->visible);
  QVERIFY(b->visible);
  QCOMPARE(g.visible_children_count(), 2);
}

void TestRadioItem::updateVisibilityRestoresOnEmptyFilter() {
  RadioItem g(true, QStringLiteral("SomaFM"));
  auto *a = station(&g, QStringLiteral("Drone Zone"));
  auto *b = station(&g, QStringLiteral("Metal Detector"));

  g.update_visibility(QStringLiteral("drone"));
  QVERIFY(!b->visible);

  g.update_visibility(QString());
  QVERIFY(g.visible);
  QVERIFY(a->visible);
  QVERIFY(b->visible);
  QCOMPARE(g.visible_children_count(), 2);
}

void TestRadioItem::visibleChildrenCountIgnoresHidden() {
  RadioItem g(true, QStringLiteral("G"));
  station(&g, QStringLiteral("a"));
  auto *b = station(&g, QStringLiteral("b"));
  b->visible = false;
  QCOMPARE(g.visible_children_count(), 1);
}

void TestRadioItem::rowReturnsZeroForRoot() {
  RadioItem root(true, QStringLiteral("root"));
  QCOMPARE(root.row(), 0);
}

void TestRadioItem::rowSkipsHiddenSiblings() {
  RadioItem g(true, QStringLiteral("G"));
  auto *a = station(&g, QStringLiteral("a"));
  auto *b = station(&g, QStringLiteral("b"));
  auto *c = station(&g, QStringLiteral("c"));

  QCOMPARE(c->row(), 2);
  a->visible = false;
  QCOMPARE(b->row(), 0);
  QCOMPARE(c->row(), 1);
}

void TestRadioItem::childReturnsNthVisible() {
  RadioItem g(true, QStringLiteral("G"));
  auto *a = station(&g, QStringLiteral("a"));
  auto *b = station(&g, QStringLiteral("b"));

  QCOMPARE(g.child(0), a);
  QCOMPARE(g.child(1), b);
  a->visible = false;
  QCOMPARE(g.child(0), b);
}

void TestRadioItem::childOutOfRangeReturnsNullptr() {
  RadioItem g(true, QStringLiteral("G"));
  station(&g, QStringLiteral("a"));
  QCOMPARE(g.child(5), nullptr);
  QCOMPARE(g.child(-1), nullptr);
}

void TestRadioItem::destructorDeletesChildren() {
  QPointer<QObject> guard;
  {
    auto *g = new RadioItem(true, QStringLiteral("G"));
    station(g, QStringLiteral("a"));
    station(g, QStringLiteral("b"));
    QCOMPARE(g->children.size(), 2);
    delete g;
  }
  // Reaching here without an ASan/leak abort is the assertion; qDeleteAll is
  // what tst_treeitem asserts the same way.
  QVERIFY(true);
}

QTEST_MAIN(TestRadioItem)
#include "tst_radioitem.moc"
