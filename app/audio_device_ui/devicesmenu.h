#ifndef DEVICESMENU_H
#define DEVICESMENU_H

#include <QObject>

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))

#include "config/local.h"

#include <QToolButton>
#include <QAction>
#include <QMenu>
#include <QWidget>
#include <QDebug>
#include <QList>
#include <QActionGroup>

namespace AudioDeviceUi {

class DevicesMenu : public QMenu
{
  Q_OBJECT
public:
  explicit DevicesMenu(QWidget *parent, Config::Local &local_c);

signals:
  void outputDeviceChanged(QByteArray id);

private:
  Config::Local &local_conf;

  bool isDefaultOutput() const;
  QByteArray currentOutput() const;

  bool saveDefaultOutput();
  bool saveOutput(QByteArray id);

private slots:
  void on_selected(QByteArray deviceid);
};

} // namespace AudioDeviceUi

#endif

#endif // DEVICESMENU_H
