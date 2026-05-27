#ifndef SORTMENU_H
#define SORTMENU_H

#include "config/global.h"
#include "sort_ui/sortingpresetsdialog.h"

#include <QObject>
#include <QToolButton>
#include <QAction>
#include <QMenu>

namespace SortUi {
  class SortMenu : public QObject {
    Q_OBJECT
  public:
    explicit SortMenu(QToolButton *button, Config::Global &global_c);

    static QList<SortingPreset> standardPresets();

    // Wire an existing menu (e.g. a menu-bar submenu) to this controller: fills
    // it now, refills on every aboutToShow (so edited presets stay current), and
    // routes activations through the same triggered() signal as the toolbar.
    void attachToMenu(QMenu *menu);

  signals:
    void triggered(const QString& criteria);

  private slots:
    void on_open();
    void on_action_triggered(QAction *action);

  private:
    QToolButton *button;
    Config::Global &global_conf;

    void populate(QMenu *menu);
    void showEditPresetsDialog();
  };
}

#endif // SORTMENU_H
