#ifndef MACMENUBAR_H
#define MACMENUBAR_H

#include <QObject>

namespace Config {
  class Global;
  class Local;
}
namespace Playback {
  class Controller;
}
namespace SortUi {
  class SortMenu;
}
class MainWindow;
class Shortcuts;
class ModusOperandi;
class QAction;

// Builds the native macOS menu bar (mpz / Playback / View / Window / Help) for
// the given window. All items live here rather than in the in-window hamburger
// menu, which is hidden on macOS (see MainWindow::setupMainMenu).
class MacMenuBar : public QObject {
  Q_OBJECT
public:
  MacMenuBar(MainWindow *window, Config::Global &global_c, Config::Local &local_c,
             Shortcuts *shortcuts, Playback::Controller *player, ModusOperandi &modus,
             SortUi::SortMenu *sort_menu, QAction *cover_toggle, QAction *lyrics_toggle);

private:
  MainWindow *window;
  Config::Global &global_conf;
  ModusOperandi &modus_operandi;
};

#endif // MACMENUBAR_H
