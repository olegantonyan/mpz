#ifndef MPZAPPLICATION_H
#define MPZAPPLICATION_H

#include <QApplication>
#include <QStringList>

class MpzApplication : public QApplication {
  Q_OBJECT
public:
  MpzApplication(int &argc, char **argv);

  QStringList drainPendingFiles();

signals:
  void filesOpened(const QStringList &files);
  void activated();

protected:
  bool event(QEvent *event) override;

private:
  QStringList pending_files;
};

#endif // MPZAPPLICATION_H
