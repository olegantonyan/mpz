#include <QtTest>
#include <QtConcurrent>

#include "backgroundtasks.h"

class TestBackgroundTasks : public QObject {
  Q_OBJECT
private slots:
  void begin_allocatesIncreasingIdsAndEmits();
  void setProgress_fillsTheTaskAndEmits();
  void setProgress_onUnknownIdIsSilent();
  void end_removesOnceAndIsIdempotent();
  void activate_onlyForClickableTasks();
  void track_endsWhenTheFutureFinishes();
};

void TestBackgroundTasks::begin_allocatesIncreasingIdsAndEmits() {
  BackgroundTasks tasks;
  QSignalSpy spy(&tasks, &BackgroundTasks::changed);

  const quint64 first = tasks.begin("one");
  const quint64 second = tasks.begin("two", true);

  QCOMPARE(first, 1u);
  QCOMPARE(second, 2u);
  QCOMPARE(spy.count(), 2);
  QCOMPARE(tasks.tasks().size(), 2);
  QCOMPARE(tasks.tasks().at(0).text, QString("one"));
  QCOMPARE(tasks.tasks().at(0).clickable, false);
  QCOMPARE(tasks.tasks().at(1).clickable, true);
}

void TestBackgroundTasks::setProgress_fillsTheTaskAndEmits() {
  BackgroundTasks tasks;
  const quint64 id = tasks.begin("scan");
  QSignalSpy spy(&tasks, &BackgroundTasks::changed);

  tasks.setProgress(id, "file.flac", 3, 10);

  QCOMPARE(spy.count(), 1);
  const auto &task = tasks.tasks().first();
  QVERIFY(task.has_progress);
  QCOMPARE(task.detail, QString("file.flac"));
  QCOMPARE(task.done, 3);
  QCOMPARE(task.total, 10);
}

void TestBackgroundTasks::setProgress_onUnknownIdIsSilent() {
  BackgroundTasks tasks;
  QSignalSpy spy(&tasks, &BackgroundTasks::changed);

  tasks.setProgress(42, "nope", 1, 2);

  QCOMPARE(spy.count(), 0);
  QVERIFY(tasks.tasks().isEmpty());
}

void TestBackgroundTasks::end_removesOnceAndIsIdempotent() {
  BackgroundTasks tasks;
  const quint64 id = tasks.begin("one");
  tasks.begin("two");
  QSignalSpy spy(&tasks, &BackgroundTasks::changed);

  tasks.end(id);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(tasks.tasks().size(), 1);
  QCOMPARE(tasks.tasks().first().text, QString("two"));

  tasks.end(id);
  QCOMPARE(spy.count(), 1);
}

void TestBackgroundTasks::activate_onlyForClickableTasks() {
  BackgroundTasks tasks;
  const quint64 plain = tasks.begin("plain");
  const quint64 clickable = tasks.begin("clickable", true);
  QSignalSpy spy(&tasks, &BackgroundTasks::activated);

  tasks.activate(plain);
  QCOMPARE(spy.count(), 0);

  tasks.activate(clickable);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.first().first().toULongLong(), clickable);
}

void TestBackgroundTasks::track_endsWhenTheFutureFinishes() {
  BackgroundTasks tasks;
  QSemaphore gate;

  tasks.track(QtConcurrent::run([&gate]() { gate.acquire(); }), "work");
  QCOMPARE(tasks.tasks().size(), 1);

  gate.release();
  QTRY_VERIFY(tasks.tasks().isEmpty());
}

QTEST_GUILESS_MAIN(TestBackgroundTasks)
#include "tst_backgroundtasks.moc"
