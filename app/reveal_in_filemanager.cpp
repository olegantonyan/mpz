#include "reveal_in_filemanager.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QString>
#include <QUrl>

#if defined(Q_OS_LINUX) && defined(MPZ_ENABLE_DBUS)
#include <QDBusConnection>
#include <QDBusMessage>
#endif

namespace {
  void openContainingDirs(const QStringList &paths) {
    QStringList seen;
    for (const auto &p : paths) {
      const auto dir = QFileInfo(p).absolutePath();
      if (!seen.contains(dir)) {
        seen << dir;
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
      }
    }
  }
}

void revealInFileManager(const QStringList &absolute_file_paths) {
  if (absolute_file_paths.isEmpty()) {
    return;
  }

#if defined(Q_OS_WIN)
  for (const auto &p : absolute_file_paths) {
    QProcess::startDetached(QStringLiteral("explorer.exe"),
                            { QStringLiteral("/select,"), QDir::toNativeSeparators(p) });
  }
#elif defined(Q_OS_MACOS)
  for (const auto &p : absolute_file_paths) {
    QProcess::startDetached(QStringLiteral("open"), { QStringLiteral("-R"), p });
  }
#elif defined(Q_OS_LINUX) && defined(MPZ_ENABLE_DBUS)
  QStringList uris;
  uris.reserve(absolute_file_paths.size());
  for (const auto &p : absolute_file_paths) {
    uris << QUrl::fromLocalFile(p).toString();
  }
  auto msg = QDBusMessage::createMethodCall(
    QStringLiteral("org.freedesktop.FileManager1"),
    QStringLiteral("/org/freedesktop/FileManager1"),
    QStringLiteral("org.freedesktop.FileManager1"),
    QStringLiteral("ShowItems"));
  msg << uris << QString();
  auto reply = QDBusConnection::sessionBus().call(msg, QDBus::Block, 2000);
  if (reply.type() == QDBusMessage::ErrorMessage) {
    openContainingDirs(absolute_file_paths);
  }
#else
  openContainingDirs(absolute_file_paths);
#endif
}
