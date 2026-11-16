#ifndef OUTPUTDEVICEBUTTON_H
#define OUTPUTDEVICEBUTTON_H

#include "config/local.h"
#ifdef ENABLE_MPZD_SUPPORT
  #include "mpzd_client/client.h"
#endif

#include <QByteArray>
#include <QHash>
#include <QPointer>
#include <QObject>
#include <QToolButton>

namespace AudioDeviceUi {
  class OutputDeviceButton : public QObject {
    Q_OBJECT

  public:
    explicit OutputDeviceButton(QToolButton *btn, Config::Local &local_c);
#ifdef ENABLE_MPZD_SUPPORT
    void setRemote(MpzdClient::Client *client);
#endif

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
    QPoint menuPos(int menu_width) const;

    QToolButton *button;
    Config::Local &local_conf;
    int text_width = 0;
#ifdef ENABLE_MPZD_SUPPORT
    QPointer<MpzdClient::Client> remote;
    QHash<QString, QString> remote_names;

    void showRemoteMenu();
    void loadRemoteNames();
#endif
  };
} // namespace AudioDeviceUi

#endif // OUTPUTDEVICEBUTTON_H
