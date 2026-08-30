#include <QtTest>
#include <QMimeData>
#include <QTemporaryDir>

#include "config/global.h"
#include "config/local.h"
#include "directory_ui/directorymodel/proxy.h"
#include "dropdirs.h"
#include "directory_ui/radiolibrary.h"
#include "modusoperandi.h"
#include "slidingbanner.h"

using DirectoryUi::DirectoryModel::Proxy;

class TestDirectoryProxy : public QObject {
  Q_OBJECT
private slots:
  void initTestCase();
  void cleanupTestCase();
  void init();
  void loadsTheLibraryRoot();
  void filter_onlyHidesTopLevelMisses();
  void radioIsOrthogonalToTheModus();
  void radioAccessorsAreEmptyWhileInactive();
  void flags_addDragForEveryRadioRow();
  void mimeData_stampsTheLibraryRoot();
  void mimeData_isNullWithoutIndexes();
  void supportedDragActions_isCopy();
  void sortBy_ignoresAnUnknownDirection();

private:
  QTemporaryDir cfg;
  QTemporaryDir library;
  SlidingBanner banner;
  std::unique_ptr<Config::Global> global;
  std::unique_ptr<Config::Local> local;
  std::unique_ptr<ModusOperandi> modus;
  std::unique_ptr<Proxy> proxy;

  QModelIndex childNamed(const QString &name) const;
};

void TestDirectoryProxy::initTestCase() {
  QVERIFY(cfg.isValid());
  QVERIFY(library.isValid());
  qputenv("MPZ_CONFIG_DIR_OVERRIDE", cfg.path().toUtf8());
  for (const QString &name : {"rock", "jazz", "blues"}) {
    QVERIFY(QDir().mkpath(library.filePath(name)));
  }
}

void TestDirectoryProxy::cleanupTestCase() {
  qunsetenv("MPZ_CONFIG_DIR_OVERRIDE");
}

void TestDirectoryProxy::init() {
  proxy.reset();
  modus.reset();
  local.reset();
  global.reset();

  global = std::make_unique<Config::Global>();
  local = std::make_unique<Config::Local>();
  modus = std::make_unique<ModusOperandi>(*local, &banner);
  proxy = std::make_unique<Proxy>(*modus, *global);

  QSignalSpy loaded(proxy.get(), &Proxy::directoryLoaded);
  proxy->loadAsync(library.path());
  QVERIFY(loaded.wait());
}

QModelIndex TestDirectoryProxy::childNamed(const QString &name) const {
  const QModelIndex root = proxy->rootIndex();
  for (int i = 0; i < proxy->rowCount(root); i++) {
    const QModelIndex idx = proxy->index(i, 0, root);
    if (QFileInfo(proxy->filePath(idx)).fileName() == name) {
      return idx;
    }
  }
  return QModelIndex();
}

void TestDirectoryProxy::loadsTheLibraryRoot() {
  QCOMPARE(proxy->rowCount(proxy->rootIndex()), 3);
  QVERIFY(childNamed("rock").isValid());
}

void TestDirectoryProxy::filter_onlyHidesTopLevelMisses() {
  proxy->filter("roc");
  QTRY_COMPARE(proxy->rowCount(proxy->rootIndex()), 1);
  QVERIFY(childNamed("rock").isValid());

  proxy->filter("");
  QTRY_COMPARE(proxy->rowCount(proxy->rootIndex()), 3);
}

void TestDirectoryProxy::radioIsOrthogonalToTheModus() {
  QVERIFY(!proxy->isRadioActive());
  auto *localfs = proxy->sourceModel();

  proxy->setRadioActive(true);
  auto *radio = proxy->sourceModel();
  QVERIFY(radio != localfs);

  modus->set(ModusOperandi::MODUS_MPD);
  QCOMPARE(proxy->sourceModel(), radio);

  modus->set(ModusOperandi::MODUS_LOCALFS);
  QCOMPARE(proxy->sourceModel(), radio);

  proxy->setRadioActive(false);
  QCOMPARE(proxy->sourceModel(), localfs);
}

void TestDirectoryProxy::radioAccessorsAreEmptyWhileInactive() {
  const QModelIndex rock = childNamed("rock");
  QVERIFY(proxy->tracksAt({rock}).isEmpty());
  QVERIFY(proxy->displayName(rock).isEmpty());
  QVERIFY(!proxy->isStation(rock));
}

void TestDirectoryProxy::flags_addDragForEveryRadioRow() {
  proxy->setRadioActive(true);
  QSignalSpy loaded(proxy.get(), &Proxy::directoryLoaded);
  proxy->loadAsync(DirectoryUi::radioLibraryPath());
  QVERIFY(loaded.count() > 0 || loaded.wait());

  QVERIFY(proxy->rowCount(proxy->rootIndex()) > 0);
  QVERIFY(proxy->flags(proxy->index(0, 0, proxy->rootIndex())) & Qt::ItemIsDragEnabled);
  QVERIFY(!(proxy->flags(QModelIndex()) & Qt::ItemIsDragEnabled));
}

void TestDirectoryProxy::mimeData_stampsTheLibraryRoot() {
  std::unique_ptr<QMimeData> mime(proxy->mimeData({childNamed("rock")}));
  QVERIFY(mime != nullptr);
  QCOMPARE(QString::fromUtf8(mime->data(DropUtil::libraryRootFormat)), library.path());
}

void TestDirectoryProxy::mimeData_isNullWithoutIndexes() {
  QCOMPARE(proxy->mimeData({}), nullptr);

  modus->set(ModusOperandi::MODUS_MPD);
  QCOMPARE(proxy->mimeData({childNamed("rock")}), nullptr);
}

void TestDirectoryProxy::supportedDragActions_isCopy() {
  QCOMPARE(proxy->supportedDragActions(), Qt::CopyAction);
}

void TestDirectoryProxy::sortBy_ignoresAnUnknownDirection() {
  proxy->sortBy("default");
  proxy->sortBy("date");
  proxy->sortBy("-date");
  proxy->sortBy("sideways");
  QCOMPARE(proxy->rowCount(proxy->rootIndex()), 3);
}

QTEST_MAIN(TestDirectoryProxy)
#include "tst_directoryproxy.moc"
