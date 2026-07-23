#include "mainwindow.h"
#include "mpzapplication.h"
#include "rnjesus.h"
#include "ipc/instance.h"
#include "track.h"
#include "streammetadata.h"
#include "config/global.h"
#include "config/local.h"
#include "config/storage.h"
#ifdef ENABLE_MPD_SUPPORT
  #include "mpd_client/entity.h"
  #include "mpd_client/song.h"
  #include "mpd_client/status.h"
  #include "mpd_client/output.h"
#endif
#ifdef ENABLE_CRASH_HANDLER
  #include "crash_handler.h"
  #include "sysinfo.h"
#endif
#ifdef Q_OS_WIN
  #include "windows/windowstaskbar.h"
#endif

#include <QDebug>
#include <QTranslator>
#include <QLocale>
#include <QStandardPaths>
#include <QDir>
#include <iostream>
#include <QPersistentModelIndex>
#include <QAbstractItemModel>

void registerMetaTypes() {
  qRegisterMetaType<Track>("Track");
  qRegisterMetaType<StreamMetaData>("StreamMetaData");
  qRegisterMetaType<std::shared_ptr<Playlist::Playlist>>("std::shared_ptr<Playlist::Playlist>");
  qRegisterMetaType<QList<QPersistentModelIndex>>("QList<QPersistentModelIndex>");
  qRegisterMetaType<QAbstractItemModel::LayoutChangeHint>("QAbstractItemModel::LayoutChangeHint");
#ifdef ENABLE_MPD_SUPPORT
  qRegisterMetaType<MpdClient::Song>("MpdClient::Song");
  qRegisterMetaType<MpdClient::Entity>("MpdClient::Entity");
  qRegisterMetaType<MpdClient::Status>("MpdClient::Status");
  qRegisterMetaType<MpdClient::Output>("MpdClient::Output");
  qRegisterMetaType<mpd_idle>("mpd_idle");
  qRegisterMetaType<QVector<MpdClient::Entity>>("QVector<MpdClient::Entity");
  qRegisterMetaType<QVector<MpdClient::Output>>("QVector<MpdClient::Output>");
  qRegisterMetaType<QVector<MpdClient::Song>>("QVector<MpdClient::Song>");
#endif
}

QStringList args(int argc, char *argv[]) {
  QStringList result;
  for (int i = 1; i < argc; i++) {
    result << argv[i];
  }
  return result;
}

void load_locale(MpzApplication &a, const QString &conf_language) {
  QString system_language = QLocale::system().name().split("_").first();
  QString lang;
  if (conf_language.isEmpty()) {
    lang = system_language;
  } else {
    lang = conf_language;
  }

  auto trans = new QTranslator(&a);
  bool load_ok = trans->load(lang, ":/app/resources/translations/");
  qDebug() << "system language:" << system_language << "config language:" << conf_language << "| translations load:" << load_ok;
  if (load_ok) {
    a.installTranslator(trans);
  } else {
    delete trans;
  }
}

int ipc_port(Config::Global &global_conf) {
  if (global_conf.ipcPort() == 0) {
    global_conf.saveIpcPort(31341);
  }
  return global_conf.ipcPort();
}

int main(int argc, char *argv[]) {
#ifdef ENABLE_CRASH_HANDLER
  mpz::install_crash_handler();
#endif
  registerMetaTypes();
  RNJesus::seed();

#if defined(Q_OS_WIN) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

  MpzApplication a(argc, argv);
  a.setApplicationName("mpz");
#ifdef ENABLE_CRASH_HANDLER
  const QString crashDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QDir().mkpath(crashDir);
  mpz::set_crash_log_path((crashDir + "/crash.log").toStdString());
#endif
#ifdef Q_OS_WIN
  WindowsTaskbar::setAppUserModelId();
#endif
#ifdef Q_OS_MACOS
  // The main window hides (not quits) on close, so don't let the app exit when
  // the last visible window — e.g. a dialog while the window is hidden — closes.
  // MainWindow::requestQuit() quits explicitly on Cmd-Q / menu Quit.
  a.setQuitOnLastWindowClosed(false);
#endif
  QString version = VERSION;
#ifdef PACKAGE_VERSION
  if (QStringLiteral(PACKAGE_VERSION) != version) {
    version = QString("%1 [%2]").arg(version, PACKAGE_VERSION);
  }
#endif
  a.setApplicationVersion(version);
  a.setApplicationDisplayName(QString("%1 v%2").arg(a.applicationName(), a.applicationVersion()));
#ifdef ENABLE_CRASH_HANDLER
  mpz::set_system_info(SysInfo::get().join("\n").toStdString());
#endif

  auto arguments = args(argc, argv);
  if (arguments.size() == 1 && arguments.first() == "--version") {
    std::cout << a.applicationVersion().toStdString() << std::endl;
    return 0;
  }

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

  // Files opened from Finder before the main window exists arrive as
  // QFileOpenEvent and queue up inside MpzApplication; merge them into
  // the initial argument list. Later FileOpen events route through the
  // same IPC signal as a second-instance file drop.
  arguments << a.drainPendingFiles();

  MainWindow w(arguments, &instance, local_conf, global_conf);
  QObject::connect(&a, &MpzApplication::filesOpened, &instance, &IPC::Instance::load_files_received);
#ifdef Q_OS_MACOS
  QObject::connect(&a, &MpzApplication::activated, &w, &MainWindow::onAppActivated);
#endif

  w.show();
  return a.exec();
}
