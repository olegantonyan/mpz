#ifndef BACKGROUNDTASKS_H
#define BACKGROUNDTASKS_H

#include <QFuture>
#include <QObject>
#include <QString>
#include <QVector>

class BackgroundTasks : public QObject {
  Q_OBJECT
public:
  struct Task {
    quint64 id = 0;
    QString text;
    QString detail;
    int done = 0;
    int total = 0;
    bool has_progress = false;
    bool clickable = false;
  };

  explicit BackgroundTasks(QObject *parent = nullptr);

  quint64 begin(const QString &text, bool clickable = false);
  void track(const QFuture<void> &work, const QString &text);
  void setProgress(quint64 id, const QString &detail, int done, int total);
  void end(quint64 id);
  void activate(quint64 id);
  const QVector<Task> &tasks() const { return tasks_; }

signals:
  void changed();
  void activated(quint64 id);

private:
  Task *find(quint64 id);

  QVector<Task> tasks_;
  quint64 next_id = 0;
};

#endif // BACKGROUNDTASKS_H
