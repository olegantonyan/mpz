#ifndef COVERART_FOLDERCOVER_H
#define COVERART_FOLDERCOVER_H

#include <QString>
#include <QStringList>

namespace CoverArt {
  namespace FolderCover {
    struct Match {
      QString file;
      int score = 0;
    };

    int score(const QString &fileName);
    Match best(const QStringList &fileNames);
  }
}

#endif // COVERART_FOLDERCOVER_H
