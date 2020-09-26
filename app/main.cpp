#include "mainwindow.h"
#include "rnjesus.h"

#include <QApplication>
#include <QDebug>
#include <QTranslator>
#include <QLocale>
#include <QTextCodec>

#include "track.h"
#include "streammetadata.h"
void registerMetaTypes() {
  qRegisterMetaType<Track>("Track");
  qRegisterMetaType<StreamMetaData>("StreamMetaData");
}

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  a.setApplicationName("mpz");
  a.setApplicationVersion(VERSION);
  a.setApplicationDisplayName(QString("%1 v%2").arg(a.applicationName()).arg(a.applicationVersion()));

  registerMetaTypes();

  RNJesus::seed();

  QString lang = QLocale::system().name().split("_").first();
  qDebug() << "system language" << lang;
  QTranslator trans;
  qDebug() << "transaltions load" << trans.load(lang, ":/translations/translations");
  a.installTranslator(&trans);

  MainWindow w;
  w.show();
  return a.exec();
}
