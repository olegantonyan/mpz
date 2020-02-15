#include "mainwindow.h"

#include <QApplication>
#include <QDebug>

#include "track.h"
#include "streammetadata.h"
void registerMetaTypes() {
  qRegisterMetaType<Track>("Track");
  qRegisterMetaType<StreamMetaData>("StreamMetaData");
}

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  a.setApplicationName("mpz");
  a.setApplicationVersion("0.0.1");

  registerMetaTypes();

  MainWindow w;
  w.show();
  return a.exec();
}
