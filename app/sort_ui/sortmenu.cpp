#include "sortmenu.h"
#include "playlist/sorter.h"

#include <QMenu>
#include <QAction>

namespace SortUi {
  SortMenu::SortMenu(QPushButton *button, Config::Global &global_c) : QObject(button), global_conf(global_c) {
    QMenu *menu = new QMenu(button);

    QAction *defau = new QAction(tr("Default"), menu);
    defau->setData(Playlist::Sorter::defaultCriteria());
    menu->addAction(defau);
    menu->addSeparator();

    for (auto i : global_conf.sortPresets()) {
      QAction *action = new QAction(i.first, menu);
      action->setData(i.second);
      menu->addAction(action);
    }

    menu->addSeparator();
    QAction *custom = new QAction(tr("Custom"), menu);
    custom->setData(42);
    menu->addAction(custom);


    connect(menu, &QMenu::triggered, [=](QAction *a) {
      if (a->data().toInt() == 42) {
        qDebug() << "custom sorting!";
      } else {
        emit triggered(a->data().toString());
      }
    });
    button->setMenu(menu);
  }

}
