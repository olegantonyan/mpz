#include "reveal_in_filemanager.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QString>
#include <QUrl>

#if defined(Q_OS_WIN)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <QMap>
#include <vector>
#include <objbase.h>
#include <shlobj.h>
#endif

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
  QMap<QString, QStringList> by_dir;
  for (const auto &p : absolute_file_paths) {
    by_dir[QFileInfo(p).absolutePath()] << p;
  }

  const HRESULT co_init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

  for (auto it = by_dir.cbegin(); it != by_dir.cend(); ++it) {
    const auto native_dir = QDir::toNativeSeparators(it.key());
    PIDLIST_ABSOLUTE folder_pidl = ILCreateFromPathW(reinterpret_cast<LPCWSTR>(native_dir.utf16()));
    if (!folder_pidl) {
      QDesktopServices::openUrl(QUrl::fromLocalFile(it.key()));
      continue;
    }

    std::vector<PIDLIST_ABSOLUTE> abs_pidls;
    std::vector<PCUITEMID_CHILD> child_pidls;
    abs_pidls.reserve(it.value().size());
    child_pidls.reserve(it.value().size());
    for (const auto &p : it.value()) {
      const auto native = QDir::toNativeSeparators(p);
      PIDLIST_ABSOLUTE pidl = ILCreateFromPathW(reinterpret_cast<LPCWSTR>(native.utf16()));
      if (pidl) {
        abs_pidls.push_back(pidl);
        child_pidls.push_back(ILFindLastID(pidl));
      }
    }

    if (!child_pidls.empty()) {
      SHOpenFolderAndSelectItems(folder_pidl, static_cast<UINT>(child_pidls.size()), child_pidls.data(), 0);
    } else {
      QDesktopServices::openUrl(QUrl::fromLocalFile(it.key()));
    }

    for (auto pidl : abs_pidls) {
      ILFree(pidl);
    }
    ILFree(folder_pidl);
  }

  if (SUCCEEDED(co_init)) {
    CoUninitialize();
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
