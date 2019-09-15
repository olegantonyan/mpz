#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileSystemModel>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void on_treeView_clicked(const QModelIndex &index);

  void on_treeView_customContextMenuRequested(const QPoint &pos);

private:
  Ui::MainWindow *ui;

  QFileSystemModel *treeViewModel;
};
#endif // MAINWINDOW_H
