#include "radiolibrary.h"

#include <QObject>
#include <QUrl>

namespace DirectoryUi {
  QString libraryPathLabel(const QString &path) {
    if (isRadioLibraryPath(path)) {
      return QObject::tr("Radio");
    }
    QUrl url(path);
    if (url.password().isEmpty()) {
      return path;
    }
    url.setPassword("***");
    return url.toString();
  }
}
