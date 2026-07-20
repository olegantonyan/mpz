#ifndef DIRECTORYUI_RADIOLIBRARY_H
#define DIRECTORYUI_RADIOLIBRARY_H

#include <QString>

namespace DirectoryUi {
  inline QString radioLibraryPath() {
    return QStringLiteral("radio://");
  }

  inline bool isRadioLibraryPath(const QString &path) {
    return path.startsWith(QStringLiteral("radio://"));
  }

  QString libraryPathLabel(const QString &path);
}

#endif // DIRECTORYUI_RADIOLIBRARY_H
