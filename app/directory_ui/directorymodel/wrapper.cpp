#include "wrapper.h"

#include <QDebug>

namespace DirectoryUi {
  namespace DirectoryModel {
    Wrapper::Wrapper(QObject *parent) : QObject(parent), localfs(nullptr), active(ActiveMode::DIRECTORY_MODEL_LOCALFS) {
      localfs = new Localfs(this);
#ifdef ENABLE_MPD_SUPPORT
      mpd = new Mpd(this);
#endif
      connect(localfs, &Localfs::directoryLoaded, this, &DirectoryModel::Wrapper::directoryLoaded);
    }

    void Wrapper::setActiveMode(ActiveMode new_active) {
#ifndef ENABLE_MPD_SUPPORT
      if (new_active == DIRECTORY_MODEL_MPD) {
        return;
      }
#endif
      active = new_active;    
    }

    void Wrapper::loadAsync(const QString &path) {
      switch (active) {
      case DIRECTORY_MODEL_MPD:
#ifdef ENABLE_MPD_SUPPORT
        mpd->loadAsync(path);
#endif
      case DIRECTORY_MODEL_LOCALFS:
      default:
        localfs->loadAsync(path);
      }
    }

    QAbstractItemModel *Wrapper::model() const {
      switch (active) {
      case DIRECTORY_MODEL_MPD:
#ifdef ENABLE_MPD_SUPPORT
        return mpd;
#endif
      case DIRECTORY_MODEL_LOCALFS:
      default:
        return localfs;
      }
    }

    QModelIndex Wrapper::rootIndex() const {
      switch (active) {
      case DIRECTORY_MODEL_MPD:
#ifdef ENABLE_MPD_SUPPORT
        return mpd->rootIndex();
#endif
      case DIRECTORY_MODEL_LOCALFS:
      default:
        return localfs->rootIndex();
      }
    }

    QString Wrapper::filePath(const QModelIndex &index) const {
      switch (active) {
      case DIRECTORY_MODEL_MPD:
#ifdef ENABLE_MPD_SUPPORT
        return mpd->filePath(index);
#endif
      case DIRECTORY_MODEL_LOCALFS:
      default:
        return localfs->filePath(index);
      }
    }

    void Wrapper::setNameFilters(const QStringList &filters) { 
      switch (active) {
      case DIRECTORY_MODEL_MPD:
#ifdef ENABLE_MPD_SUPPORT
        mpd->setNameFilters(filters);
#endif
      case DIRECTORY_MODEL_LOCALFS:
      default:
        localfs->setNameFilters(filters);
      }
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

    Wrapper::ActiveMode Wrapper::activeMode() const {
      return active;
    }
  }
}
