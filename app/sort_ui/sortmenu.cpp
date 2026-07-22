#include "sortmenu.h"
#include "playlist/sorter.h"
#include "sort_ui/sortingpresetsdialog.h"

#include <QMenu>
#include <QAction>

static const int EDIT_PRESETS_QACTION_DATA = 42;

namespace SortUi {
  SortMenu::SortMenu(QToolButton *butn, Config::Global &global_c) : QObject(butn), button(butn), global_conf(global_c) {
    connect(button, &QToolButton::clicked, this, &SortMenu::on_open);
    //button->setMenu(new QMenu(button)); // to show small arrow
  }

  QList<SortingPreset> SortMenu::standardPresets(){
    QList<SortingPreset> result;

    result << SortingPreset("", "Title");
    result << SortingPreset("", "-Title");
    result << SortingPreset("", "Artist");
    result << SortingPreset("", "-Artist");
    result << SortingPreset("", "Album / Title");
    result << SortingPreset("", "-Album / Title");
    result << SortingPreset("", "Arist / Album / TrackNumber / Filename / Title");

    return result;
  }

  void SortMenu::populate(QMenu *menu) {
    menu->clear();

    QAction *defau = new QAction(tr("Default"), menu);
    defau->setData(Playlist::Sorter::defaultCriteria());
    menu->addAction(defau);
    menu->addSeparator();

    if (global_conf.sortPresets().isEmpty()) {
      global_conf.saveSortPresets(SortUi::SortMenu::standardPresets());
    }

    for (const auto &i : global_conf.sortPresets()) {
      QAction *action = new QAction(i.first.isEmpty() ? i.second : i.first, menu);
      action->setData(i.second);
      menu->addAction(action);
    }

    menu->addSeparator();
    QAction *custom = new QAction(tr("Edit presets"), menu);
    custom->setData(EDIT_PRESETS_QACTION_DATA);
    menu->addAction(custom);
  }

  void SortMenu::attachToMenu(QMenu *menu) {
    connect(menu, &QMenu::triggered, this, &SortMenu::on_action_triggered);
    connect(menu, &QMenu::aboutToShow, this, [this, menu]() { populate(menu); });
    populate(menu);
  }

  void SortMenu::on_open() {
    QMenu menu;
    connect(&menu, &QMenu::triggered, this, &SortMenu::on_action_triggered);
    populate(&menu);

    int menu_width = menu.sizeHint().width();
    int x = button->width() - menu_width;
    int y = button->height();
    QPoint pos(button->mapToGlobal(QPoint(x, y)));
    menu.exec(pos);
  }

  void SortMenu::on_action_triggered(QAction *action) {
    if (action->data().toInt() == EDIT_PRESETS_QACTION_DATA) {
      showEditPresetsDialog();
    } else {
      emit triggered(action->data().toString());
    }
  }

  void SortMenu::showEditPresetsDialog() {
    SortingPresetsDialog *dlg = new SortingPresetsDialog(global_conf.sortPresets());
    dlg->setModal(false);
    connect(dlg, &SortingPresetsDialog::finished, this, [=](int result) {
      if (result == QDialog::Accepted) {
        auto presets = dlg->currentPresets();
        global_conf.saveSortPresets(presets);
      }
      dlg->deleteLater();
    });
    connect(dlg, &SortingPresetsDialog::triggeredSort, this, &SortMenu::triggered);
    dlg->show();
  }
}
