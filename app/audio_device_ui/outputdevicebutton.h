#ifndef OUTPUTDEVICEBUTTON_H
#define OUTPUTDEVICEBUTTON_H

#include "config/local.h"

#include <QByteArray>
#include <QObject>
#include <QToolButton>

namespace AudioDeviceUi {
  class OutputDeviceButton : public QObject {
    Q_OBJECT

  public:
    explicit OutputDeviceButton(QToolButton *btn, Config::Local &local_c);

  signals:
    void outputDeviceChanged(QByteArray id);

  public slots:
    void refresh();

  protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

  private slots:
    void on_buttonClicked();

  private:
    void updateSize();

    QToolButton *button;
    Config::Local &local_conf;
    int text_width = 0;
  };
} // namespace AudioDeviceUi

#endif // OUTPUTDEVICEBUTTON_H
