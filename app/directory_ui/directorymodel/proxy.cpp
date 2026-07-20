#include "proxy.h"

#include <QDebug>

namespace DirectoryUi {
  namespace DirectoryModel {
    Proxy::Proxy(ModusOperandi &modus, Config::Global &global_cfg, QObject *parent) : QSortFilterProxyModel(parent), modus_operandi(modus), global_conf(global_cfg) {
      localfs = new Localfs(this);
      radio = new Radio(global_cfg, this);
      connect(radio, &Radio::directoryLoaded, this, &DirectoryModel::Proxy::directoryLoaded);
#ifdef ENABLE_MPD_SUPPORT
      mpd = new Mpd(modus.mpd_client, this);
      connect(mpd, &Mpd::directoryLoaded, this, &DirectoryModel::Proxy::directoryLoaded);
      connect(&modus_operandi, &ModusOperandi::mpdReady, mpd, &DirectoryModel::Mpd::onMpdReady);
      connect(&modus_operandi, &ModusOperandi::mpdLost, mpd, &DirectoryModel::Mpd::onMpdLost);
#endif
      connect(localfs, &Localfs::directoryLoaded, this, &DirectoryModel::Proxy::directoryLoaded);
      connect(&modus_operandi, &ModusOperandi::changed, this, &Proxy::switchTo);
      switchTo(modus_operandi.get());
    }

    void Proxy::setRadioActive(bool active) {
      radio_active = active;
      if (active) {
        setSourceModel(radio);
      } else {
        switchTo(modus_operandi.get());
      }
    }

    void Proxy::switchTo(ModusOperandi::ActiveMode new_mode) {
      // While radio is shown, a modus change must not swap the source model.
      if (radio_active) {
        return;
      }
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
      if (radio_active) {
        radio->loadAsync(path);
        return;
      }
      switch (modus_operandi.get()) {
      case ModusOperandi::MODUS_MPD:
#ifdef ENABLE_MPD_SUPPORT
        if (loadAsyncMpdOnce) { // next time it will react to the signal
          mpd->loadAsync(path);
          loadAsyncMpdOnce = false;
        }
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

      if (radio_active) {
        return mapFromSource(radio->rootIndex());
      }
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

      if (radio_active) {
        return radio->filePath(mapToSource(index));
      }
      switch (modus_operandi.get()) {
      case ModusOperandi::MODUS_MPD:
#ifdef ENABLE_MPD_SUPPORT
        result = mpd->filePath(mapToSource(index));
#endif
        break;
      case ModusOperandi::MODUS_LOCALFS:
      default:
        result = localfs->filePath(mapToSource(index));
        break;
      }

      return result;
    }

    void Proxy::filter(const QString &term) {
      if (radio_active) {
        radio->filter(term);
        return;
      }
      switch (modus_operandi.get()) {
      case ModusOperandi::MODUS_MPD:
#ifdef ENABLE_MPD_SUPPORT
        mpd->filter(term);
#endif
        break;
      case ModusOperandi::MODUS_LOCALFS:
      default:
        if (global_conf.libraryFilterScope() == "top_level_only") {
          // Top-level-only mode: proxy does the filtering. Clear any legacy name filter
          // that may have been set in all-levels mode before the user switched.
          localfs->setNameFilters(QStringList());
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
          beginFilterChange();
          filter_term = term;
          endFilterChange(QSortFilterProxyModel::Direction::Rows);
#else
          filter_term = term;
          invalidateFilter();
#endif
        } else {
          // All-levels mode (default; also covers empty/unknown stored value):
          // QFileSystemModel filters at every level via setNameFilters. Make sure the
          // proxy-level filter is cleared so it doesn't double up if the user just switched.
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
          beginFilterChange();
          filter_term.clear();
          endFilterChange(QSortFilterProxyModel::Direction::Rows);
#else
          filter_term.clear();
          invalidateFilter();
#endif
          if (term.isEmpty()) {
            localfs->setNameFilters(QStringList());
          } else {
            localfs->setNameFilters(QStringList() << QString("*%1*").arg(term));
          }
        }
      }
    }

    bool Proxy::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
      if (filter_term.isEmpty()) {
        return true;
      }
      // Only LocalFS rows pass through this filter; MPD manages its own visibility in Mpd::filter().
      if (sourceModel() != localfs) {
        return true;
      }
      // Filter only at the top level (children of rootIndex); deeper rows are always accepted
      // so that expanding a matched top-level dir shows its full contents.
      if (source_parent != localfs->rootIndex()) {
        return true;
      }
      const QString name = localfs->fileName(localfs->index(source_row, 0, source_parent));
      return name.contains(filter_term, Qt::CaseInsensitive);
    }

    QVector<Track> Proxy::tracksAt(const QModelIndexList &indexes) const {
      if (!radio_active) {
        return QVector<Track>();
      }
      QModelIndexList source_indexes;
      for (const auto &i : indexes) {
        source_indexes << mapToSource(i);
      }
      return radio->tracksAt(source_indexes);
    }

    QString Proxy::displayName(const QModelIndex &index) const {
      if (!radio_active) {
        return QString();
      }
      return radio->displayName(mapToSource(index));
    }

    bool Proxy::isStation(const QModelIndex &index) const {
      if (!radio_active) {
        return false;
      }
      return radio->isStation(mapToSource(index));
    }

    void Proxy::sortBy(const QString &direction) {
      if (direction.toLower() == "default") {
        sourceModel()->sort(0, Qt::AscendingOrder);
      } else if (direction.toLower() == "date") {
        sourceModel()->sort(3, Qt::AscendingOrder);
      } else if (direction.toLower() == "-date") {
        sourceModel()->sort(3, Qt::DescendingOrder);
      }
    }
  }
}
