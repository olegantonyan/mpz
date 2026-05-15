#include <QtTest>

#include "directory_ui/directorymodel/mpd/treeitem.h"

using DirectoryUi::DirectoryModel::TreeItem;

class TestTreeItem : public QObject {
  Q_OBJECT
private slots:
  void constructorSetsFields();
  void setPathWithNameUpdatesName();
  void matchEmptyFilterAlwaysTrue();
  void matchCaseInsensitive();
  void matchNoSubstring();
  void updateVisibilitySetsAndReturnsVisible();
  void visibleChildrenCountIgnoresHidden();
  void rowReturnsZeroForRoot();
  void rowSkipsHiddenSiblings();
  void childReturnsNthVisible();
  void childOutOfRangeReturnsNullptr();
  void destructorDeletesChildren();
};

void TestTreeItem::constructorSetsFields() {
  TreeItem t(true, QStringLiteral("/music/albums"), 1700000000);
  QCOMPARE(t.is_directory, true);
  QCOMPARE(t.path, QStringLiteral("/music/albums"));
  QCOMPARE(t.name, QStringLiteral("albums"));
  QCOMPARE(t.last_modified, time_t{1700000000});
  QVERIFY(t.visible);
  QVERIFY(!t.loaded);
  QCOMPARE(t.parent, static_cast<TreeItem*>(nullptr));
  QVERIFY(t.children.isEmpty());
}

void TestTreeItem::setPathWithNameUpdatesName() {
  TreeItem t(true, QStringLiteral("/a"));
  t.set_path_with_name(QStringLiteral("/music/artists/Foo"));
  QCOMPARE(t.path, QStringLiteral("/music/artists/Foo"));
  QCOMPARE(t.name, QStringLiteral("Foo"));
}

void TestTreeItem::matchEmptyFilterAlwaysTrue() {
  TreeItem t(false, QStringLiteral("/some/song.flac"));
  QVERIFY(t.match(QString()));
  QVERIFY(t.match(QStringLiteral("")));
}

void TestTreeItem::matchCaseInsensitive() {
  TreeItem t(true, QStringLiteral("/music/Albums"));
  QVERIFY(t.match(QStringLiteral("alb")));
  QVERIFY(t.match(QStringLiteral("ALB")));
  QVERIFY(t.match(QStringLiteral("AlBuMs")));
}

void TestTreeItem::matchNoSubstring() {
  TreeItem t(true, QStringLiteral("/music/Albums"));
  QVERIFY(!t.match(QStringLiteral("xyz")));
}

void TestTreeItem::updateVisibilitySetsAndReturnsVisible() {
  TreeItem t(true, QStringLiteral("/music/jazz"));
  QVERIFY(t.update_visibility(QStringLiteral("jazz")));
  QVERIFY(t.visible);
  QVERIFY(!t.update_visibility(QStringLiteral("xyz")));
  QVERIFY(!t.visible);
}

void TestTreeItem::visibleChildrenCountIgnoresHidden() {
  TreeItem root(true, QStringLiteral("/"));
  auto a = new TreeItem(true, QStringLiteral("/a"), 0, &root);
  auto b = new TreeItem(true, QStringLiteral("/b"), 0, &root);
  auto c = new TreeItem(true, QStringLiteral("/c"), 0, &root);
  root.children = { a, b, c };
  QCOMPARE(root.visible_children_count(), 3);
  b->visible = false;
  QCOMPARE(root.visible_children_count(), 2);
  a->visible = false;
  c->visible = false;
  QCOMPARE(root.visible_children_count(), 0);
}

void TestTreeItem::rowReturnsZeroForRoot() {
  TreeItem root(true, QStringLiteral("/"));
  QCOMPARE(root.row(), 0);
}

void TestTreeItem::rowSkipsHiddenSiblings() {
  TreeItem root(true, QStringLiteral("/"));
  auto a = new TreeItem(true, QStringLiteral("/a"), 0, &root);
  auto b = new TreeItem(true, QStringLiteral("/b"), 0, &root);
  auto c = new TreeItem(true, QStringLiteral("/c"), 0, &root);
  auto d = new TreeItem(true, QStringLiteral("/d"), 0, &root);
  root.children = { a, b, c, d };
  QCOMPARE(a->row(), 0);
  QCOMPARE(b->row(), 1);
  QCOMPARE(c->row(), 2);
  QCOMPARE(d->row(), 3);
  b->visible = false;
  QCOMPARE(c->row(), 1);
  QCOMPARE(d->row(), 2);
}

void TestTreeItem::childReturnsNthVisible() {
  TreeItem root(true, QStringLiteral("/"));
  auto a = new TreeItem(true, QStringLiteral("/a"), 0, &root);
  auto b = new TreeItem(true, QStringLiteral("/b"), 0, &root);
  auto c = new TreeItem(true, QStringLiteral("/c"), 0, &root);
  root.children = { a, b, c };
  QCOMPARE(root.child(0), a);
  QCOMPARE(root.child(1), b);
  QCOMPARE(root.child(2), c);
  b->visible = false;
  QCOMPARE(root.child(0), a);
  QCOMPARE(root.child(1), c);
}

void TestTreeItem::childOutOfRangeReturnsNullptr() {
  TreeItem root(true, QStringLiteral("/"));
  auto a = new TreeItem(true, QStringLiteral("/a"), 0, &root);
  root.children = { a };
  QCOMPARE(root.child(1), static_cast<TreeItem*>(nullptr));
  QCOMPARE(root.child(99), static_cast<TreeItem*>(nullptr));
}

void TestTreeItem::destructorDeletesChildren() {
  // We can't observe deletion directly, but constructing/destroying a tree
  // with several nested children should not leak under ASAN. This test
  // exercises that path; the assertion is that the program doesn't crash.
  {
    TreeItem root(true, QStringLiteral("/"));
    auto a = new TreeItem(true, QStringLiteral("/a"), 0, &root);
    auto b = new TreeItem(true, QStringLiteral("/b"), 0, &root);
    auto a1 = new TreeItem(false, QStringLiteral("/a/song.flac"), 0, a);
    a->children = { a1 };
    root.children = { a, b };
  }
  QVERIFY(true);
}

QTEST_GUILESS_MAIN(TestTreeItem)
#include "tst_treeitem.moc"
