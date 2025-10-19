#include "wrapper.h"

#include <QDebug>

namespace DirectoryUi {
  namespace DirectoryModel {
  Wrapper::Wrapper(QObject *parent) : QObject(parent), localfs(nullptr) {
      localfs = new Localfs(this);
#ifdef ENABLE_MPD_SUPPORT
      mpd = new Mpd(this);
#endif
      connect(localfs, &Localfs::directoryLoaded, this, &DirectoryModel::Wrapper::directoryLoaded);
    }

    void Wrapper::loadAsync(const QString &path) {
      localfs->loadAsync(path);
    }

    QAbstractItemModel *Wrapper::model() const {
      return localfs;
    }

    QModelIndex Wrapper::rootIndex() const {
      return localfs->index(localfs->rootPath());
    }

    QString Wrapper::rootPath() const {
      return localfs->rootPath();
    }

    QString Wrapper::filePath(const QModelIndex &index) const {
      return localfs->filePath(index);
    }

    void Wrapper::setNameFilters(const QStringList &filters) {
      localfs->setNameFilters(filters);
    }

    void Wrapper::sortBy(const QString &direction) {
      if (direction.toLower() == "default") {
        model()->sort(0, Qt::AscendingOrder);
      } else if (direction.toLower() == "date") {
        model()->sort(3, Qt::AscendingOrder);
      } else if (direction.toLower() == "- date") {
        model()->sort(3, Qt::DescendingOrder);
      }
    }
  }
}
