#include "backgroundtasksbutton.h"

#include <QFontMetrics>
#include <QPalette>
#include <QTimer>
#include <QVBoxLayout>

namespace {
  const int kRowWidth = 280;
}

namespace PrivateBackgroundTasks {
  Row::Row(QWidget *parent) : QFrame(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 2, 8, 6);
    layout->setSpacing(2);

    separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    layout->addWidget(separator);

    text = new QLabel(this);
    layout->addWidget(text);

    detail = new QLabel(this);
    detail->setStyleSheet("color: gray;");
    layout->addWidget(detail);

    progress = new QProgressBar(this);
    progress->setFormat(QStringLiteral("%v / %m"));
    layout->addWidget(progress);

    setFixedWidth(kRowWidth);
    setBackgroundRole(QPalette::AlternateBase);
  }

  void Row::set(const BackgroundTasks::Task &task, bool separated) {
    task_id = task.id;
    clickable = task.clickable;

    separator->setVisible(separated);
    text->setText(task.text);
    detail->setText(fontMetrics().elidedText(task.detail, Qt::ElideMiddle, kRowWidth - 16));
    detail->setVisible(task.has_progress);
    progress->setVisible(task.has_progress);
    if (task.has_progress) {
      progress->setRange(0, task.total);
      progress->setValue(task.done);
      progress->setTextVisible(task.total > 0);
    }
    setCursor(clickable ? Qt::PointingHandCursor : Qt::ArrowCursor);
  }

  void Row::mouseReleaseEvent(QMouseEvent *event) {
    if (clickable && event->button() == Qt::LeftButton && rect().contains(event->position().toPoint())) {
      emit clicked();
    }
    QFrame::mouseReleaseEvent(event);
  }

  void Row::enterEvent(QEnterEvent *event) {
    setAutoFillBackground(clickable);
    QFrame::enterEvent(event);
  }

  void Row::leaveEvent(QEvent *event) {
    setAutoFillBackground(false);
    QFrame::leaveEvent(event);
  }
}

BackgroundTasksButton::BackgroundTasksButton(QWidget *parent) : QToolButton(parent) {
  setAutoRaise(true);
  setFocusPolicy(Qt::NoFocus);
  setFixedSize(24, 24);
  hide();

  spinner = new LoadingSpinner(this);
  spinner->setAttribute(Qt::WA_TransparentForMouseEvents);

  menu = new QMenu(this);

  connect(this, &QToolButton::clicked, this, &BackgroundTasksButton::openMenu);
}

void BackgroundTasksButton::setTasks(BackgroundTasks *t) {
  tasks = t;
  connect(tasks, &BackgroundTasks::changed, this, &BackgroundTasksButton::scheduleRefresh);
  refresh();
}

void BackgroundTasksButton::openMenu() {
  menu->popup(mapToGlobal(QPoint(width() - menu->sizeHint().width(), height())));
}

PrivateBackgroundTasks::Row *BackgroundTasksButton::rowAt(int index) const {
  return static_cast<PrivateBackgroundTasks::Row *>(actions.at(index)->defaultWidget());
}

void BackgroundTasksButton::scheduleRefresh() {
  if (refresh_scheduled) {
    return;
  }
  refresh_scheduled = true;
  QTimer::singleShot(0, this, [this]() {
    refresh_scheduled = false;
    refresh();
  });
}

void BackgroundTasksButton::refresh() {
  const QVector<BackgroundTasks::Task> &list = tasks->tasks();

  while (actions.size() < list.size()) {
    auto *row = new PrivateBackgroundTasks::Row;
    connect(row, &PrivateBackgroundTasks::Row::clicked, this, [this, row]() {
      menu->hide();
      tasks->activate(row->taskId());
    });
    auto *action = new QWidgetAction(menu);
    action->setDefaultWidget(row);
    menu->addAction(action);
    actions.append(action);
  }
  while (actions.size() > list.size()) {
    auto *action = actions.takeLast();
    menu->removeAction(action);
    action->deleteLater();
  }
  for (int i = 0; i < list.size(); i++) {
    rowAt(i)->set(list.at(i), i > 0);
  }

  setVisible(!list.isEmpty());
  if (list.isEmpty()) {
    spinner->stop();
    menu->hide();
    return;
  }

  spinner->start();
  setToolTip(list.size() == 1 ? list.first().text
                              : tr("%n background operations", "", list.size()));
}
