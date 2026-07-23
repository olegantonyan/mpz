#include "mainmenu.h"
#include "about_ui/aboutdialog.h"
#include "feedback_ui/feedbackform.h"
#include "settings_ui/settingsdialog.h"

#include <QDebug>
#include <QMenu>
#include <QAction>

MainMenu::MainMenu(QToolButton *btn, Config::Global &global_c, Config::Local &local_c, ModusOperandi &modus) : QObject(btn), button(btn), global_conf(global_c), local_conf(local_c), modus_operandi(modus) {
  connect(button, &QToolButton::clicked, this, &MainMenu::on_open);
}

void MainMenu::setViewActions(const QList<QAction *> &actions) {
  view_actions = actions;
}

void MainMenu::on_open() {
  QMenu menu;

  QAction settings(tr("Settings…"));
  QAction equalizer(tr("Equalizer…"));
  QAction lpog(tr("Playback log"));
  QAction about(tr("About mpz"));
  QAction quit(tr("Quit"));
  QAction feedback(tr("Got feedback?"));
  QAction shortcuts(tr("Keyboard shortcuts"));
  QAction mpdupdate(tr("mpd update"));

  connect(&settings, &QAction::triggered, this, [this]() {
    SettingsDialog dlg(global_conf, local_conf, button->parentWidget());
    connect(&dlg, &SettingsDialog::trayIconToggled, this, &MainMenu::toggleTrayIcon);
    dlg.exec();
  });
  connect(&about, &QAction::triggered, [=]() {
    AboutDialog(global_conf, local_conf).exec();
  });
  connect(&quit, &QAction::triggered, this, &MainMenu::exit);
  connect(&lpog, &QAction::triggered, this, &MainMenu::openPlaybackLog);
  connect(&feedback, &QAction::triggered, [=]() {
    FeedbackForm(local_conf).exec();
  });
  connect(&shortcuts, &QAction::triggered, this, &MainMenu::openShortcuts);
#ifdef ENABLE_GAPLESS
  equalizer.setEnabled(!global_conf.disableGapless());
  connect(&equalizer, &QAction::triggered, this, &MainMenu::openEqualizer);
#endif
#ifdef ENABLE_MPD_SUPPORT
  connect(&mpdupdate, &QAction::triggered, this, [=]() {
    modus_operandi.mpd_client.updateDb();
  });
#endif

  menu.addAction(&settings);
#ifdef ENABLE_GAPLESS
  menu.addAction(&equalizer);
#endif
  if (!view_actions.isEmpty()) {
    menu.addSeparator();
    for (auto *action : std::as_const(view_actions)) {
      menu.addAction(action);
    }
  }
#ifdef ENABLE_MPD_SUPPORT
  if (modus_operandi.get() == ModusOperandi::MODUS_MPD) {
    menu.addAction(&mpdupdate);
  }
#endif
  menu.addSeparator();
  menu.addAction(&lpog);
  menu.addAction(&about);
  menu.addSeparator();
  menu.addAction(&feedback);
  menu.addAction(&shortcuts);
  menu.addSeparator();
  menu.addAction(&quit);

  int menu_width = menu.sizeHint().width();
  int x = button->width() - menu_width;
  int y = button->height();
  QPoint pos(button->mapToGlobal(QPoint(x, y)));

  menu.exec(pos);
}
