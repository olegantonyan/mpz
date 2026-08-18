#include "backgroundtasks.h"

#include <QFutureWatcher>

BackgroundTasks::BackgroundTasks(QObject *parent) : QObject(parent) {
}

quint64 BackgroundTasks::begin(const QString &text, bool clickable) {
  Task task;
  task.id = ++next_id;
  task.text = text;
  task.clickable = clickable;
  tasks_.append(task);
  emit changed();
  return task.id;
}

void BackgroundTasks::track(const QFuture<void> &work, const QString &text) {
  const quint64 id = begin(text);
  auto *watcher = new QFutureWatcher<void>(this);
  connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, id]() {
    end(id);
    watcher->deleteLater();
  });
  watcher->setFuture(work);
}

void BackgroundTasks::setProgress(quint64 id, const QString &detail, int done, int total) {
  Task *task = find(id);
  if (task == nullptr) {
    return;
  }
  task->detail = detail;
  task->done = done;
  task->total = total;
  task->has_progress = true;
  emit changed();
}

void BackgroundTasks::end(quint64 id) {
  for (int i = 0; i < tasks_.size(); i++) {
    if (tasks_.at(i).id == id) {
      tasks_.removeAt(i);
      emit changed();
      return;
    }
  }
}

void BackgroundTasks::activate(quint64 id) {
  const Task *task = find(id);
  if (task != nullptr && task->clickable) {
    emit activated(id);
  }
}

BackgroundTasks::Task *BackgroundTasks::find(quint64 id) {
  for (auto &task : tasks_) {
    if (task.id == id) {
      return &task;
    }
  }
  return nullptr;
}
