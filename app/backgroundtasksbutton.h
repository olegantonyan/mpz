#ifndef BACKGROUNDTASKSBUTTON_H
#define BACKGROUNDTASKSBUTTON_H

#include "backgroundtasks.h"
#include "loadingspinner.h"

#include <QEnterEvent>
#include <QEvent>
#include <QFrame>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QProgressBar>
#include <QToolButton>
#include <QVector>
#include <QWidget>
#include <QWidgetAction>

namespace PrivateBackgroundTasks {
  class Row : public QFrame {
    Q_OBJECT
  public:
    explicit Row(QWidget *parent = nullptr);

    void set(const BackgroundTasks::Task &task, bool separated);
    quint64 taskId() const { return task_id; }

  signals:
    void clicked();

  protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

  private:
    QFrame *separator;
    QLabel *text;
    QLabel *detail;
    QProgressBar *progress;
    quint64 task_id = 0;
    bool clickable = false;
  };
}

class BackgroundTasksButton : public QToolButton {
  Q_OBJECT
public:
  explicit BackgroundTasksButton(QWidget *parent = nullptr);

  void setTasks(BackgroundTasks *t);

private:
  void scheduleRefresh();
  void refresh();
  void openMenu();
  PrivateBackgroundTasks::Row *rowAt(int index) const;

  BackgroundTasks *tasks = nullptr;
  LoadingSpinner *spinner = nullptr;
  QMenu *menu = nullptr;
  QVector<QWidgetAction *> actions;
  bool refresh_scheduled = false;
};

#endif // BACKGROUNDTASKSBUTTON_H
