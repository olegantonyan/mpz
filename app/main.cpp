#include "mainwindow.h"
#include "rnjesus.h"
#include "ipc/instance.h"
#include "track.h"
#include "streammetadata.h"
#include "config/global.h"
#include "config/local.h"

#include <QApplication>
#include <QDebug>
#include <QTranslator>
#include <QLocale>
#include <QTextCodec>

void registerMetaTypes() {
  qRegisterMetaType<Track>("Track");
  qRegisterMetaType<StreamMetaData>("StreamMetaData");
}

QStringList args(int argc, char *argv[]) {
  QStringList result;
  for (int i = 1; i < argc; i++) {
    result << argv[i];
  }
  return result;
}

void load_locale(QApplication &a, const QString &conf_language) {
  QString system_language = QLocale::system().name().split("_").first();
  QString lang;
  if (conf_language.isEmpty()) {
    lang = system_language;
  } else {
    lang = conf_language;
  }

  auto trans = new QTranslator;
  bool load_ok = trans->load(lang, ":/app/resources/translations/");
  qDebug() << "system language:" << system_language << "config language:" << conf_language << "| transaltions load:" << load_ok;
  if (load_ok) {
    a.installTranslator(trans);
  }
}

int main(int argc, char *argv[]) {
  registerMetaTypes();
  RNJesus::seed();

  QApplication a(argc, argv);
  a.setApplicationName("mpz");
  a.setApplicationVersion(VERSION);
  a.setApplicationDisplayName(QString("%1 v%2").arg(a.applicationName()).arg(a.applicationVersion()));

  Config::Global global_conf;
  Config::Local local_conf;

  load_locale(a, global_conf.language());

  IPC::Instance instance;
  if (instance.isAnotherRunning()) {
    qDebug() << "another instance is running, reusing it";
    QVariantMap m;
    m.insert("files", args(argc, argv));
    return instance.send(m) == true ? 0 : 1;
  } else {
    qDebug() << "first instance started";
    instance.start();
  }

  MainWindow w(args(argc, argv), local_conf, global_conf);

  w.show();
  return a.exec();
}
