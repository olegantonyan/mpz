#include "reveal_in_filemanager.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QMap>
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
  // Group by directory so each Finder window shows all of that folder's
  // selected items at once, mirroring the Windows branch above. Driving Finder
  // via osascript is the only way to multi-select; `open -R` reveals a single
  // item per invocation and opens a fresh window each time.
  QMap<QString, QStringList> by_dir;
  for (const auto &p : absolute_file_paths) {
    by_dir[QFileInfo(p).absolutePath()] << p;
  }

  // Escape for an AppleScript string literal. No shell is involved (startDetached
  // passes args straight to osascript), so only backslash and double-quote matter.
  const auto escape = [](const QString &s) {
    QString out = s;
    out.replace('\\', QStringLiteral("\\\\"));
    out.replace('"', QStringLiteral("\\\""));
    return out;
  };

  QStringList fallback;
  for (auto it = by_dir.cbegin(); it != by_dir.cend(); ++it) {
    QStringList items;
    for (const auto &p : it.value()) {
      items << QStringLiteral("POSIX file \"%1\"").arg(escape(p));
    }
    const QStringList args = {
      QStringLiteral("-e"), QStringLiteral("tell application \"Finder\""),
      QStringLiteral("-e"), QStringLiteral("activate"),
      QStringLiteral("-e"), QStringLiteral("open (POSIX file \"%1\")").arg(escape(it.key())),
      QStringLiteral("-e"), QStringLiteral("select {%1}").arg(items.join(QStringLiteral(", "))),
      QStringLiteral("-e"), QStringLiteral("end tell")
    };
    if (!QProcess::startDetached(QStringLiteral("osascript"), args)) {
      fallback << it.value();
    }
  }

  // osascript could not be launched at all — fall back to the old per-file reveal.
  for (const auto &p : fallback) {
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
