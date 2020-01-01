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

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

private:
  Ui::MainWindow *ui;
  DirectoryUi::Controller *library;
  PlaylistsUi::Controller *playlists;
  PlaylistUi::Controller *playlist;
  Playback::Controller *player;
  Playback::Dispatch *dispatch;
  Config::Local local_conf;
  Config::Global global_conf;
  BusySpinner *spinner;
  TrayIcon *trayicon;
  VolumeControl *volume;

  void loadUiSettings();

protected:
  void closeEvent(QCloseEvent *event) override;

private slots:
  void on_menuButton_clicked();
  void updateVolume(int value);
};
#endif // MAINWINDOW_H
