#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "directory_ui/directorycontroller.h"
#include "playlists_ui/playlistscontroller.h"
#include "playlist_ui/playlistcontroller.h"
#include "playback/playbackcontroller.h"
#include "config/local.h"
#include "config/global.h"
#include "playback/dispatch.h"
#include "backgroundtasks.h"
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
  #include "audio_device_ui/outputdevicebutton.h"
#endif
#ifdef ENABLE_GAPLESS
  #include "replaygain/manager.h"
#endif
#include "modusoperandi.h"
#include "slidingbanner.h"
#include "coverart/coverartwidget.h"
#include "lyrics/lyricswidget.h"

#include <QAction>
#include <QDockWidget>
#include <QMainWindow>
#include <QMenu>
#include <QPointer>
#include <QTimer>
#include <QToolBar>
#include <QtGlobal>

#if defined(MPRIS_ENABLE)
  #include "dbus/mpris.h"
#endif

#ifdef Q_OS_MACOS
  #include "macos/macmediacontrols.h"
  #include "macos/macdockmenu.h"
  #include "macos/macdockicon.h"
  #include "macos/macmenubar.h"
#endif

#ifdef SMTC_ENABLE
  #include "windows/windowsmediacontrols.h"
#endif

#ifdef Q_OS_WIN
  #include "windows/windowstaskbar.h"
#endif

#ifdef ENABLE_MPD_SUPPORT
  #include "playback/mpd/playbackorder.h"
#endif

#ifdef ENABLE_GAPLESS
  #include "equalizer_ui/equalizerdialog.h"
  #include "replaygain_ui/replaygaindialog.h"
  #include "replaygain_ui/statusmenu.h"
#endif

#if defined(ENABLE_UPDATE_CHECK)
  #include "update_check/updatechecker.h"
#endif

#if defined(ENABLE_CRASH_HANDLER)
  #include "feedback_ui/feedbacksender.h"
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
  void showWindow();
  void requestQuit();
#ifdef Q_OS_MACOS
  void onAppActivated();
#endif

private:
  Ui::MainWindow *ui = nullptr;
  DirectoryUi::Controller *library = nullptr;
  PlaylistsUi::Controller *playlists = nullptr;
  PlaylistUi::Controller *playlist = nullptr;
  Playback::Controller *player = nullptr;
  Playback::Dispatch *dispatch = nullptr;
  Config::Local &local_conf;
  Config::Global &global_conf;
  BackgroundTasks *tasks = nullptr;
  TrayIcon *trayicon = nullptr;
  VolumeControl *volume = nullptr;
  MainMenu *main_menu = nullptr;
  StatusBarLabel *status_label = nullptr;
  QLabel *status_label_right = nullptr;
#if defined(ENABLE_UPDATE_CHECK)
  QLabel *status_label_update = nullptr;
  UpdateChecker *update_checker = nullptr;
#endif
#if defined(ENABLE_CRASH_HANDLER)
  FeedbackSender *crash_sender = nullptr;
#endif
#if defined(MPRIS_ENABLE)
  Mpris *mpris = nullptr;
#endif
#ifdef Q_OS_MACOS
  MacMediaControls *mac_media = nullptr;
  MacDockMenu *mac_dock = nullptr;
  MacDockIcon *mac_dock_icon = nullptr;
  MacMenuBar *mac_menubar = nullptr;
#endif
#ifdef SMTC_ENABLE
  WindowsMediaControls *win_media = nullptr;
#endif
#ifdef Q_OS_WIN
  WindowsTaskbar *win_taskbar = nullptr;
#endif
  Shortcuts *shortcuts = nullptr;
#ifdef ENABLE_GAPLESS
  ReplayGain::Manager *replay_gain = nullptr;
  QLabel *status_label_replaygain = nullptr;
  ReplayGainUi::StatusMenu *rg_status_menu = nullptr;
  Track playing_track;
  quint64 rg_task = 0;
#endif
  PlaybackLogUi::Controller *playback_log = nullptr;
  SortUi::SortMenu *sort_menu = nullptr;
#ifdef ENABLE_DEVICES_MENU
  AudioDeviceUi::OutputDeviceButton *output_device_button = nullptr;
#endif
#ifdef ENABLE_GAPLESS
  EqualizerUi::EqualizerDialog *eq_dialog = nullptr;
  QPointer<ReplayGainUi::ReplayGainDialog> rg_dialog;
#endif
  SleepLock *sleep_lock = nullptr;
  SlidingBanner *banner = nullptr;
  QToolBar *controls_toolbar = nullptr;
  QAction *lock_toolbar_action = nullptr;
  QDockWidget *cover_dock = nullptr;
  QDockWidget *lyrics_dock = nullptr;
  CoverArt::Widget *cover_widget = nullptr;
  Lyrics::Widget *lyrics_widget = nullptr;
  ModusOperandi modus_operandi;
#ifdef ENABLE_MPD_SUPPORT
  Playback::Mpd::PlaybackOrder *mpd_order = nullptr;
#endif
  bool quitting = false;
  bool autoplay_created_playlist = false;
  QMetaObject::Connection preload_conn;
  QTimer *preload_deadline = nullptr;

  int streamBuffer();
  void setupUiSettings();
  void setupOrderCombobox();
  void setupPerPlaylistOrderCombobox();
  void setupFollowCursorCheckbox();
  void setupVolumeControl();
  void setupControlsToolBar();
  void setupDockWidgets();
  void openTrackInfo(const Track &track);
  void setupTrayIcon();
  void setupPlaybackDispatch();
  void setupStatusBar();
#if defined(ENABLE_UPDATE_CHECK)
  void setupUpdateChecker();
#endif
#if defined(ENABLE_CRASH_HANDLER)
  void setupCrashReporter();
#endif
#if defined(MPRIS_ENABLE)
  void setupMpris();
#endif
  void setupShortcuts();
  void setupMainMenu();
#ifdef Q_OS_MACOS
  void setupMacMenuBar();
  void setupMacMediaControls();
  void setupMacDockMenu();
  void setupMacDockIcon();
#endif
#ifdef SMTC_ENABLE
  void setupWindowsMediaControls();
#endif
#ifdef Q_OS_WIN
  void setupWindowsTaskbar();
#endif
  void setupWindowTitle();
  void setupPlaybackLog();
  void setupSortMenu();
  void setupSleepLock();
  void setupOutputDevice();
#ifdef ENABLE_GAPLESS
  void setupEqualizer();
  void openEqualizerDialog();
  void applyEqForDevice(const QByteArray &device_id);
  void setupReplayGain();
  void openReplayGainDialog();
  void updateReplayGainStatus();
#endif
#ifdef ENABLE_MPD_SUPPORT
  void setupMpdOrder();
#endif

  void preloadPlaylist(const QStringList &args);
  void cancelPreload();

protected:
  void closeEvent(QCloseEvent *event) override;
  QMenu *createPopupMenu() override;
};
#endif // MAINWINDOW_H
