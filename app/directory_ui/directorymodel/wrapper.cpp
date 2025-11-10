#include "wrapper.h"

#include <QDebug>

namespace DirectoryUi {
  namespace DirectoryModel {
    Wrapper::Wrapper(QObject *parent) : QObject(parent), localfs(nullptr) {
      localfs = new Localfs(this);
#ifdef ENABLE_MPD_SUPPORT
      mpd = new Mpd(this);
      connect(mpd, &Mpd::directoryLoaded, this, &DirectoryModel::Wrapper::directoryLoaded);
#endif
      connect(localfs, &Localfs::directoryLoaded, this, &DirectoryModel::Wrapper::directoryLoaded);
    }

    void Wrapper::loadAsync(const QString &path) {
      switch (ModusOperandi::instance().get()) {
      case ModusOperandi::MODUS_MPD:
#ifdef ENABLE_MPD_SUPPORT
        mpd->loadAsync(path);
#endif
      case ModusOperandi::MODUS_LOCALFS:
      default:
        localfs->loadAsync(path);
      }
    }

    QAbstractItemModel *Wrapper::model() const {
      switch (ModusOperandi::instance().get()) {
      case ModusOperandi::MODUS_MPD:
#ifdef ENABLE_MPD_SUPPORT
        return mpd;
#endif
      case ModusOperandi::MODUS_LOCALFS:
      default:
        return localfs;
      }
    }

    QModelIndex Wrapper::rootIndex() const {
      switch (ModusOperandi::instance().get()) {
      case ModusOperandi::MODUS_MPD:
#ifdef ENABLE_MPD_SUPPORT
        return mpd->rootIndex();
#endif
      case ModusOperandi::MODUS_LOCALFS:
      default:
        return localfs->rootIndex();
      }
    }

    QString Wrapper::filePath(const QModelIndex &index) const {
      switch (ModusOperandi::instance().get()) {
      case ModusOperandi::MODUS_MPD:
#ifdef ENABLE_MPD_SUPPORT
        return mpd->filePath(index);
#endif
      case ModusOperandi::MODUS_LOCALFS:
      default:
        return localfs->filePath(index);
      }
    }

    void Wrapper::filter(const QString &term) {
      switch (ModusOperandi::instance().get()) {
      case ModusOperandi::MODUS_MPD:
#ifdef ENABLE_MPD_SUPPORT
        mpd->filter(term);
#endif
      case ModusOperandi::MODUS_LOCALFS:
      default:
        if (term.isEmpty()) {
          localfs->setNameFilters(QStringList());
        } else {
          QString wc = QString("*%1*").arg(term);
          localfs->setNameFilters(QStringList() << wc);
        }
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
  }
}
