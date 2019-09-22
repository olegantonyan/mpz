#include "mainwindow.h"

#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);

  QCoreApplication::setOrganizationName("mpz");
  QCoreApplication::setOrganizationDomain("github.com/olegantonyan/mpz");
  QCoreApplication::setApplicationName("mpz");

  MainWindow w;
  w.show();
  return a.exec();
}
