#include "fixture.h"

#include "audio_device_ui/devicesmenu.h"
#include "audio_device_ui/outputdevicename.h"

#include <QAction>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QWidget>

// Only enumerates devices, never opens one, so it is the same headless or not.
class TestDevicesMenu : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();
  void cleanup();
  void emptyIdIsNamedDefault();
  void unknownIdEchoesBackTheRawId();
  void rememberedDescriptionSurvivesUnplugging();
  void menuAlwaysOffersDefaultFirst();
  void configuredButAbsentDeviceGetsADisabledPlaceholder();
  void selectingDefaultPersistsAnEmptyId();

private:
  GuiTest::ConfigDir config;
  QWidget host;
  std::unique_ptr<Config::Local> local;
  int counter = 0;

  QByteArray uniqueId();
};

void TestDevicesMenu::initTestCase() {
  QVERIFY(config.init());
}

void TestDevicesMenu::init() {
  local = std::make_unique<Config::Local>();
}

void TestDevicesMenu::cleanup() {
  local.reset();
  QFile::remove(config.path() + "/local.yml");
  QVERIFY(config.init());
}

QByteArray TestDevicesMenu::uniqueId() {
  // The description cache is process-wide, so ids must not repeat across tests.
  return QByteArray("absent-device-") + QByteArray::number(++counter);
}

void TestDevicesMenu::emptyIdIsNamedDefault() {
  const QString name = AudioDeviceUi::outputDeviceName(QByteArray());
  QVERIFY2(name.startsWith("Default"), qPrintable(name));

  const QString description = QMediaDevices::defaultAudioOutput().description();
  if (description.isEmpty()) {
    QCOMPARE(name, QString("Default"));
  } else {
    QCOMPARE(name, "Default [" + description + "]");
  }
}

void TestDevicesMenu::unknownIdEchoesBackTheRawId() {
  const QByteArray id = uniqueId();
  QCOMPARE(AudioDeviceUi::outputDeviceName(id), QString::fromLatin1(id));
}

void TestDevicesMenu::rememberedDescriptionSurvivesUnplugging() {
  const QByteArray id = uniqueId();
  AudioDeviceUi::rememberOutputDevice(id, "Studio Monitors");

  // The device is not in audioOutputs(), which is the whole point of the cache.
  QCOMPARE(AudioDeviceUi::outputDeviceName(id), QString("Studio Monitors"));
}

void TestDevicesMenu::menuAlwaysOffersDefaultFirst() {
  AudioDeviceUi::DevicesMenu menu(&host, *local);
  const auto actions = menu.actions();

  QVERIFY(actions.size() >= 2);
  QCOMPARE(actions.at(0)->text(), AudioDeviceUi::outputDeviceName(QByteArray()));
  QVERIFY(actions.at(0)->isChecked());
  QVERIFY(actions.at(1)->isSeparator());
  QCOMPARE(actions.size(), QMediaDevices::audioOutputs().size() + 2);
}

void TestDevicesMenu::configuredButAbsentDeviceGetsADisabledPlaceholder() {
  const QByteArray id = uniqueId();
  AudioDeviceUi::rememberOutputDevice(id, "Unplugged DAC");
  QVERIFY(local->saveOutputDeviceId(id));

  AudioDeviceUi::DevicesMenu menu(&host, *local);
  auto *placeholder = menu.actions().last();

  QCOMPARE(placeholder->text(), QString("Unplugged DAC"));
  QVERIFY(placeholder->isChecked());
  QVERIFY(!placeholder->isEnabled());
  QVERIFY(!menu.actions().at(0)->isChecked());
}

void TestDevicesMenu::selectingDefaultPersistsAnEmptyId() {
  QVERIFY(local->saveOutputDeviceId(uniqueId()));
  AudioDeviceUi::DevicesMenu menu(&host, *local);
  QSignalSpy spy(&menu, &AudioDeviceUi::DevicesMenu::outputDeviceChanged);

  menu.actions().at(0)->trigger();

  QCOMPARE(spy.count(), 1);
  QVERIFY(spy.first().first().toByteArray().isEmpty());
  QVERIFY(local->outputDeviceId().isEmpty());
}

MPZ_GUI_TEST_MAIN(TestDevicesMenu)
#include "tst_devicesmenu.moc"
