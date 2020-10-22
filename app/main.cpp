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

  qDebug() << "starting" << a.applicationDisplayName();


  QString lang = QLocale::system().name().split("_").first();
  QTranslator trans;
  bool load_ok = trans.load(lang, ":/app/resources/translations/");
  qDebug() << "system language:" << lang << "| transaltions load:" << load_ok;
  if (load_ok) {
    a.installTranslator(&trans);
  }


  QStringList args;
  for (int i = 1; i < argc; i++) {
    args << argv[i];
  }
  //args << "/mnt/storage/music/ZZ Top/tejas/10_zz_top_asleep_in_the_desert.mp3";

  MainWindow w(args);
  w.show();
  return a.exec();
}
