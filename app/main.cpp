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
#include <iostream>
#include <QDir>

#ifdef SENTRY_DSN
  #include <sentry.h>
#endif

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

int ipc_port(Config::Global &global_conf) {
  if (global_conf.ipcPort() == 0) {
    global_conf.saveIpcPort(31341);
  }
  return global_conf.ipcPort();
}

int main(int argc, char *argv[]) {
  //qputenv("QT_MEDIA_BACKEND", "ffmpeg");
  registerMetaTypes();
  RNJesus::seed();

  QApplication a(argc, argv);
  a.setApplicationName("mpz");
  a.setApplicationVersion(VERSION);
  a.setApplicationDisplayName(QString("%1 v%2").arg(a.applicationName()).arg(a.applicationVersion()));

  auto arguments = args(argc, argv);
  if (arguments.size() == 1 && arguments.first() == "--version") {
    std::cout << a.applicationVersion().toStdString() << std::endl;
    return 0;
  }

#ifdef SENTRY_DSN
  sentry_options_t *options = sentry_options_new();
  sentry_options_set_dsn(options, SENTRY_DSN);
  auto crashes_path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
  sentry_options_set_database_path(options, crashes_path.toLocal8Bit().data());
  auto release = QString("%1@%2").arg(a.applicationName(), a.applicationVersion());
  sentry_options_set_release(options, release.toLocal8Bit().data());
  sentry_options_set_debug(options, 1);
  auto handler = QDir::toNativeSeparators(a.applicationDirPath() + "/mpz_crashpad_handler");;
  sentry_options_set_handler_path(options, QDir::toNativeSeparators(handler).toLocal8Bit().data());
  sentry_init(options);
  auto sentryClose = qScopeGuard([] { sentry_close(); });
#endif

  Config::Global global_conf;
  Config::Local local_conf;

  load_locale(a, global_conf.language());

  IPC::Instance instance(ipc_port(global_conf));
  if (global_conf.singleInstance()) {
    int another_pid = instance.anotherPid();
    if (another_pid > 0) {
      qDebug() << "reusing another instance with pid" << another_pid;
      return instance.load_files_send(arguments) == true ? 0 : 1;
    } else {
      instance.start();
    }
  }

  MainWindow w(arguments, &instance, local_conf, global_conf);

  w.show();
  return a.exec();
}
