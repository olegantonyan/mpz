#include "mainwindow.h"

#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  a.setApplicationName("mpz");

  MainWindow w;
  w.show();
  return a.exec();
}
