#include "proxy.h"

#include <QDebug>

namespace DirectoryUi {
  namespace DirectoryModel {
    Proxy::Proxy(ModusOperandi &modus, QObject *parent) : QSortFilterProxyModel(parent), modus_operandi(modus) {
      localfs = new Localfs(this);
#ifdef ENABLE_MPD_SUPPORT
      mpd = new Mpd(modus.mpd_connection, this);
      connect(mpd, &Mpd::directoryLoaded, this, &DirectoryModel::Proxy::directoryLoaded);
#endif
      connect(localfs, &Localfs::directoryLoaded, this, &DirectoryModel::Proxy::directoryLoaded);
      connect(&modus_operandi, &ModusOperandi::changed, this, &Proxy::swtich_to);
      swtich_to(modus_operandi.get());
    }

    void Proxy::swtich_to(ModusOperandi::ActiveMode new_mode) {
      switch (new_mode) {
      case ModusOperandi::MODUS_MPD:
#ifdef ENABLE_MPD_SUPPORT
        setSourceModel(mpd);
        break;
#endif
      case ModusOperandi::MODUS_LOCALFS:
      default:
        setSourceModel(localfs);
        break;
      }
    }

    void Proxy::loadAsync(const QString &path) {
      switch (modus_operandi.get()) {
      case ModusOperandi::MODUS_MPD:
#ifdef ENABLE_MPD_SUPPORT
        mpd->loadAsync(path);
#endif
        break;
      case ModusOperandi::MODUS_LOCALFS:
      default:
        localfs->loadAsync(path);
        break;
      }
    }

    QModelIndex Proxy::rootIndex() const {
      QModelIndex result;

      switch (modus_operandi.get()) {
      case ModusOperandi::MODUS_MPD:
#ifdef ENABLE_MPD_SUPPORT
        result = mapFromSource(mpd->rootIndex());
#endif
        break;
      case ModusOperandi::MODUS_LOCALFS:
      default:
        result = mapFromSource(localfs->rootIndex());
        break;
      }

      return result;
    }

    QString Proxy::filePath(const QModelIndex &index) const {
      QString result;

      switch (modus_operandi.get()) {
      case ModusOperandi::MODUS_MPD:
#ifdef ENABLE_MPD_SUPPORT
        result = mpd->filePath(mapFromSource(index));
#endif
        break;
      case ModusOperandi::MODUS_LOCALFS:
      default:
        result = localfs->filePath(mapFromSource(index));
        break;
      }

      return result;
    }

    void Proxy::filter(const QString &term) {
      switch (modus_operandi.get()) {
      case ModusOperandi::MODUS_MPD:
#ifdef ENABLE_MPD_SUPPORT
        mpd->filter(term);
#endif
        break;
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

    void Proxy::sortBy(const QString &direction) {
      if (direction.toLower() == "default") {
        sourceModel()->sort(0, Qt::AscendingOrder);
      } else if (direction.toLower() == "date") {
        sourceModel()->sort(3, Qt::AscendingOrder);
      } else if (direction.toLower() == "- date") {
        sourceModel()->sort(3, Qt::DescendingOrder);
      }
    }
  }
}
