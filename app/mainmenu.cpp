#include "mainmenu.h"

#include <QMenu>
#include <QAction>
#include <QDesktopServices>
#include <QUrl>

MainMenu::MainMenu(QToolButton *btn, QObject *parent) : QObject(parent), button(btn) {
  connect(button, &QToolButton::clicked, this, &MainMenu::on_buttonClicked);
}

void MainMenu::on_buttonClicked() {
  QMenu menu;
  QAction about("About");
  QAction quit("Quit");
  connect(&about, &QAction::triggered, [=]() {
    QDesktopServices::openUrl(QUrl("https://github.com/olegantonyan/mpz"));
  });
  connect(&quit, &QAction::triggered, this, &MainMenu::exit);

  menu.addAction(&about);
  menu.addSeparator();
  menu.addAction(&quit);

  int menu_width = menu.sizeHint().width();
  int x = button->width() - menu_width;
  int y = button->height();
  QPoint pos(button->mapToGlobal(QPoint(x, y)));

  menu.exec(pos);
}
