#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);

  QCoreApplication::setOrganizationName("Oleg Antonyan");
  QCoreApplication::setOrganizationDomain("github.com/olegantonyan/mpz");
  QCoreApplication::setApplicationName("mpz");

  MainWindow w;
  w.show();
  return a.exec();
}
