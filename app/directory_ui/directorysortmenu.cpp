#include "directory_ui/directorysortmenu.h"

#include <QMenu>
#include <QAction>

namespace DirectoryUi {
  SortMenu::SortMenu(QToolButton *butn) : QObject(butn), button(butn) {
    connect(button, &QToolButton::clicked, this, &SortMenu::on_open);
  }

  void SortMenu::on_open() {
    QMenu menu;
    connect(&menu, &QMenu::triggered, [=](QAction *action) {
      emit triggered(action->data().toString());
    });

    QAction defau(tr("Default (Name)"));
    defau.setData("Default");
    menu.addAction(&defau);
    menu.addSeparator();

    QAction date_p("Date");
    date_p.setData("Date");
    menu.addAction(&date_p);

    QAction date_n("- Date");
    date_n.setData("- Date");
    menu.addAction(&date_n);

    int menu_width = menu.sizeHint().width();
    int x = button->width() - menu_width;
    int y = button->height();
    QPoint pos(button->mapToGlobal(QPoint(x, y)));
    menu.exec(pos);
  }
}
