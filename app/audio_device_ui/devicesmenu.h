#ifndef DEVICESMENU_H
#define DEVICESMENU_H

#include "config/local.h"

#include <QObject>
#include <QToolButton>
#include <QAction>
#include <QMenu>
#include <QWidget>
#include <QDebug>
#include <QList>
#include <QActionGroup>
#include <QHash>

namespace AudioDeviceUi {
  class DevicesMenu : public QMenu {
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

  static QHash<QByteArray, QString> devices_id_description_cache;

} // namespace AudioDeviceUi

#endif // DEVICESMENU_H
