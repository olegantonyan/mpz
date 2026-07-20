#ifndef DIRECTORYUI_RADIOLIBRARY_H
#define DIRECTORYUI_RADIOLIBRARY_H

#include <QString>

namespace DirectoryUi {
  // The radio station browser is a view state of the library tree, not an app
  // mode: it lives in the library path list as this sentinel and resolves to
  // MODUS_LOCALFS, so playback and everything downstream stay local.
  inline QString radioLibraryPath() {
    return QStringLiteral("radio://");
  }

  inline bool isRadioLibraryPath(const QString &path) {
    return path.startsWith(QStringLiteral("radio://"));
  }

  // Combobox / settings label: "Radio" for the sentinel, password-redacted for
  // mpd://, the path itself otherwise. The stored path is left untouched.
  QString libraryPathLabel(const QString &path);
}

#endif // DIRECTORYUI_RADIOLIBRARY_H
