#ifndef MACUPDATER_H
#define MACUPDATER_H

#include <QObject>

class MacUpdater : public QObject {
  Q_OBJECT
public:
  explicit MacUpdater(QObject *parent = nullptr);
  ~MacUpdater() override;

public slots:
  void checkForUpdates();

private:
  void *updater_controller = nullptr;
};

#endif // MACUPDATER_H
