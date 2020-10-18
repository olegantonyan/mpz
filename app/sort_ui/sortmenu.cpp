#include "sort_ui/sortmenu.h"
#include "playlist/sorter.h"
#include "sort_ui/sortingpresets.h"

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
    QAction *custom = new QAction(tr("Edit presets"), menu);
    custom->setData(42);
    menu->addAction(custom);


    connect(menu, &QMenu::triggered, [=](QAction *a) {
      if (a->data().toInt() == 42) {
        showEditPresetsDialog();
      } else {
        emit triggered(a->data().toString());
      }
    });
    button->setMenu(menu);
  }

  void SortMenu::showEditPresetsDialog() {
    SortingPresets *dlg = new SortingPresets(global_conf.sortPresets());
    dlg->setModal(false);
    connect(dlg, &SortingPresets::finished, [=](int result) {
      if (result == QDialog::Accepted) {
        qDebug() << dlg->currentPresets();
      }
      dlg->deleteLater();
    });
    connect(dlg, &SortingPresets::triggeredSort, this, &SortMenu::triggered);
    dlg->show();
  }
}
