#include "mainmenu.h"
#include "aboutdialog.h"

#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QDesktopServices>
#include <QUrl>

MainMenu::MainMenu(QToolButton *btn, Config::Global &global_c) : QObject(btn), button(btn), global_conf(global_c) {
  connect(button, &QToolButton::clicked, this, &MainMenu::on_open);
}

void MainMenu::on_open() {
  QMenu menu;
  QAction trayicon("Tray icon");
  trayicon.setCheckable(true);
  trayicon.setChecked(global_conf.trayIconEnabled());
  QAction about("About");
  QAction quit("Quit");
  connect(&about, &QAction::triggered, [=]() {
    AboutDialog().exec();
  });
  connect(&quit, &QAction::triggered, this, &MainMenu::exit);
  connect(&trayicon, &QAction::triggered, [&]() {
    global_conf.saveTrayIconEnabled(trayicon.isChecked());
    emit toggleTrayIcon();
  });

  menu.addAction(&trayicon);
  menu.addAction(&about);
  menu.addSeparator();
  menu.addAction(&quit);

  int menu_width = menu.sizeHint().width();
  int x = button->width() - menu_width;
  int y = button->height();
  QPoint pos(button->mapToGlobal(QPoint(x, y)));

  menu.exec(pos);
}
