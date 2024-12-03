#include "mainmenu.h"
#include "about_ui/aboutdialog.h"
#include "feedback_ui/feedbackform.h"

#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QDesktopServices>
#include <QUrl>

MainMenu::MainMenu(QToolButton *btn, Config::Global &global_c, Config::Local &local_c) : QObject(btn), button(btn), global_conf(global_c), local_conf(local_c) {
  connect(button, &QToolButton::clicked, this, &MainMenu::on_open);
}

void MainMenu::on_open() {
  QMenu menu;
  QAction trayicon(tr("Tray icon"));
  trayicon.setCheckable(true);
  trayicon.setChecked(global_conf.trayIconEnabled());
  QAction minimize_to_tray(tr("Minimize to tray"));
  minimize_to_tray.setCheckable(true);
  minimize_to_tray.setEnabled(trayicon.isChecked());
  minimize_to_tray.setChecked(global_conf.minimizeToTray());

  QAction lpog(tr("Playback log"));
  QAction about(tr("About mpz"));
  QAction quit(tr("Quit"));
  QAction feedback(tr("Got feedback?"));
  QAction shortcuts(tr("Keyboard shortcuts"));
  QAction saves(tr("Save settings"));
  QAction confdir(tr("Open config directory"));

  connect(&about, &QAction::triggered, [=]() {
    AboutDialog().exec();
  });
  connect(&quit, &QAction::triggered, this, &MainMenu::exit);
  connect(&lpog, &QAction::triggered, this, &MainMenu::openPlaybackLog);
  connect(&trayicon, &QAction::triggered, [=](bool checked) {
    global_conf.saveTrayIconEnabled(checked);
    emit toggleTrayIcon();
  });
  connect(&minimize_to_tray, &QAction::triggered, [=](bool checked) {
    global_conf.saveMinimizeToTray(checked);
  });
  connect(&feedback, &QAction::triggered, [=]() {
    FeedbackForm().exec();
  });
  connect(&shortcuts, &QAction::triggered, this, &MainMenu::openShortcuts);
  connect(&saves, &QAction::triggered, [=]() {
    global_conf.sync();
    local_conf.sync();
  });
  connect(&confdir, &QAction::triggered, [=]() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(Config::Storage::configPath()));
  });

  menu.addAction(&trayicon);
  menu.addAction(&minimize_to_tray);
  menu.addAction(&saves);
  menu.addAction(&confdir);
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
