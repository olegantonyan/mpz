#include "sort_ui/sortmenu.h"
#include "playlist/sorter.h"
#include "sort_ui/sortingpresetsdialog.h"

#include <QMenu>
#include <QAction>

static const int EDIT_PRESETS_QACTION_DATA = 42;

namespace SortUi {
  namespace {
    // Legacy fix-up for the "Arist" typo shipped in the default presets up to 2.1.5. Delete once old configs are gone.
    bool repairAristTypo(QList<SortingPreset> &presets) {
      bool repaired = false;
      for (auto &i : presets) {
        if (i.second.contains("Arist")) {
          i.second.replace("Arist", "Artist");
          repaired = true;
        }
      }
      return repaired;
    }
  }

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
    result << SortingPreset("", "Artist / Album / TrackNumber / Filename / Title");

    return result;
  }

  void SortMenu::populate(QMenu *menu) {
    menu->clear();

    QAction *defau = new QAction(tr("Default"), menu);
    defau->setData(Playlist::Sorter::defaultCriteria());
    menu->addAction(defau);
    menu->addSeparator();

    auto presets = global_conf.sortPresets();

    if (presets.isEmpty()) {
      presets = standardPresets();
      global_conf.saveSortPresets(presets);
    } else if (repairAristTypo(presets)) {
      global_conf.saveSortPresets(presets);
    }

    for (const auto &i : std::as_const(presets)) {
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
