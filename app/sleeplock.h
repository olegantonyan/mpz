#ifndef SHUTDOWNLOCK_H
#define SHUTDOWNLOCK_H

#include <QObject>
#include <QProcess>

class SleepLock : public QObject {
  Q_OBJECT
public:
  explicit SleepLock(QObject *parent = nullptr);

public slots:
  void activate(bool state);

private:
  QProcess proc;
};

#endif // SHUTDOWNLOCK_H
