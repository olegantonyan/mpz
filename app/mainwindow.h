#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "directory_ui/directoryview.h"
#include "playlists_ui/playlistsview.h"
#include "playlist_ui/playlistview.h"

#include <QMainWindow>

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
};
#endif // MAINWINDOW_H
