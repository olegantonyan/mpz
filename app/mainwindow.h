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
#ifdef ENABLE_DEVICES_MENU
  #include "audio_device_ui/devicesmenu.h"
#endif
#include "modusoperandi.h"
#include "slidingbanner.h"

#include <QMainWindow>
#include <QtGlobal>

#if defined(MPRIS_ENABLE)
  #include "dbus/mpris.h"
#endif

#ifdef ENABLE_MPD_SUPPORT
  #include "playback/mpd/playbackorder.h"
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
  Ui::MainWindow *ui = nullptr;
  DirectoryUi::Controller *library = nullptr;
  PlaylistsUi::Controller *playlists = nullptr;
  PlaylistUi::Controller *playlist = nullptr;
  Playback::Controller *player = nullptr;
  Playback::Dispatch *dispatch = nullptr;
  Config::Local &local_conf;
  Config::Global &global_conf;
  BusySpinner *spinner = nullptr;
  TrayIcon *trayicon = nullptr;
  VolumeControl *volume = nullptr;
  MainMenu *main_menu = nullptr;
  StatusBarLabel *status_label = nullptr;
  QLabel *status_label_right = nullptr;
#if defined(MPRIS_ENABLE)
  Mpris *mpris = nullptr;
#endif
  Shortcuts *shortcuts = nullptr;
  PlaybackLogUi::Controller *playback_log = nullptr;
  SortUi::SortMenu *sort_menu = nullptr;
  SleepLock *sleep_lock = nullptr;
  SlidingBanner *banner = nullptr;
  ModusOperandi modus_operandi;
#ifdef ENABLE_MPD_SUPPORT
  Playback::Mpd::PlaybackOrder *mpd_order = nullptr;
#endif

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
  void setupOutputDevice();
#ifdef ENABLE_MPD_SUPPORT
  void setupMpdOrder();
#endif

  void preloadPlaylist(const QStringList &args);

protected:
  void closeEvent(QCloseEvent *event) override;
  void changeEvent(QEvent *) override;
};
#endif // MAINWINDOW_H
