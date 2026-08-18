#include "replaygain_ui/statusmenu.h"

#include <QAction>
#include <QList>
#include <QPair>

namespace ReplayGainUi {
  StatusMenu::StatusMenu(ReplayGain::Manager &r, Config::Global &global_c, QWidget *parent) :
    QMenu(parent), rg(r), global_conf(global_c) {
    connect(this, &QMenu::aboutToShow, this, &StatusMenu::populate);
    populate();
  }

  void StatusMenu::populate() {
    clear();
    delete action_group;
    action_group = new QActionGroup(this);

    const ReplayGain::Mode current = rg.settings().mode;
    const QList<QPair<ReplayGain::Mode, QString>> modes = {
      {ReplayGain::Mode::Off, tr("Off")},
      {ReplayGain::Mode::Track, tr("Track gain")},
      {ReplayGain::Mode::Album, tr("Album gain")}
    };

    for (const auto &m : modes) {
      auto action = new QAction(m.second, action_group);
      action->setCheckable(true);
      action->setChecked(m.first == current);
      const ReplayGain::Mode mode = m.first;
      connect(action, &QAction::triggered, this, [this, mode](bool checked) {
        if (checked) {
          selectMode(mode);
        }
      });
      addAction(action);
    }

    addSeparator();
    auto open = addAction(tr("ReplayGain…"));
    connect(open, &QAction::triggered, this, &StatusMenu::openDialog);
  }

  void StatusMenu::selectMode(ReplayGain::Mode mode) {
    ReplayGain::Settings s = rg.settings();
    if (s.mode == mode) {
      return;
    }
    s.mode = mode;
    rg.setSettings(s);
    global_conf.sync();
  }
} // namespace ReplayGainUi
