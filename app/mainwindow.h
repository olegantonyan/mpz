#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "directory_ui/directorycontroller.h"
#include "playlists_ui/playlistscontroller.h"
#include "playlist_ui/playlistcontroller.h"
#include "playback/playbackcontroller.h"
#include "config/local.h"
#include "config/global.h"
#include "playback/dispatch.h"
#include "busyspinner.h"
#include "trayicon.h"
#include "volumecontrol.h"
#include "mainmenu.h"
#include "statusbarlabel.h"
#include "shortcuts.h"
#include "playback_log_ui/playbackloguicontroller.h"
#include "sort_ui/sortmenu.h"
#include "ipc/instance.h"
#include "sleeplock.h"

#include <QMainWindow>
#include <QtGlobal>

#if defined(MPRIS_ENABLE)
  #include "dbus/mpris.h"
#endif

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(const QStringList &args, IPC::Instance *instance, Config::Local &local_c, Config::Global &global_c, QWidget *parent = nullptr);
  ~MainWindow() override;

public slots:
  void toggleHidden();

private:
  Ui::MainWindow *ui;
  DirectoryUi::Controller *library;
  PlaylistsUi::Controller *playlists;
  PlaylistUi::Controller *playlist;
  Playback::Controller *player;
  Playback::Dispatch *dispatch;
  Config::Local &local_conf;
  Config::Global &global_conf;
  BusySpinner *spinner;
  TrayIcon *trayicon;
  VolumeControl *volume;
  MainMenu *main_menu;
  StatusBarLabel *status_label;
  QLabel *status_label_right;
#if defined(MPRIS_ENABLE)
  Mpris *mpris;
#endif
  Shortcuts *shortcuts;
  PlaybackLogUi::Controller *playback_log;
  SortUi::SortMenu *sort_menu;
  SleepLock *sleep_lock;

  int streamBuffer();
  void setupUiSettings();
  void setupOrderCombobox();
  void setupPerPlaylistOrderCombobox();
  void setupFollowCursorCheckbox();
  void setupVolumeControl();
  void setupTrayIcon();
  void setupPlaybackDispatch();
  void setupStatusBar();
#if defined(MPRIS_ENABLE)
  void setupMpris();
#endif
  void setupShortcuts();
  void setupMainMenu();
  void setupWindowTitle();
  void setupPlaybackLog();
  void setupSortMenu();
  void setupSleepLock();

  void preloadPlaylist(const QStringList &args);
  
protected:
  void closeEvent(QCloseEvent *event) override;
  void changeEvent(QEvent *) override;
};
#endif // MAINWINDOW_H
