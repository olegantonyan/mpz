#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "directory_ui/directoryview.h"
#include "playlists_ui/playlistsview.h"
#include "playlist_ui/playlistview.h"
#include "playback/playbackview.h"
#include "config/local.h"
#include "config/global.h"
#include "playback/dispatch.h"
#include "busyspinner.h"
#include "trayicon.h"

#include <QMainWindow>
#include <memory>
#include <QSystemTrayIcon>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private:
  Ui::MainWindow *ui;
  DirectoryUi::View *library;
  PlaylistsUi::View *playlists;
  PlaylistUi::View *playlist;
  Playback::View *player;
  Playback::Dispatch *dispatch;
  Config::Local local_conf;
  Config::Global global_conf;
  BusySpinner *spinner;
  TrayIcon *trayicon;

  void loadUiSettings();

protected:
  void closeEvent(QCloseEvent *event);
private slots:
  void on_menuButton_clicked();
};
#endif // MAINWINDOW_H
