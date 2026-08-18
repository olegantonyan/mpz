#ifndef REPLAYGAIN_UI_STATUSMENU_H
#define REPLAYGAIN_UI_STATUSMENU_H

#include "config/global.h"
#include "replaygain/manager.h"

#include <QActionGroup>
#include <QMenu>

namespace ReplayGainUi {
  class StatusMenu : public QMenu {
    Q_OBJECT
  public:
    explicit StatusMenu(ReplayGain::Manager &r, Config::Global &global_c, QWidget *parent = nullptr);

  signals:
    void openDialog();

  private:
    void populate();
    void selectMode(ReplayGain::Mode mode);

    ReplayGain::Manager &rg;
    Config::Global &global_conf;
    QActionGroup *action_group = nullptr;
  };
}

#endif // REPLAYGAIN_UI_STATUSMENU_H
