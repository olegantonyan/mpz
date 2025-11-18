#include "playlistproxyfiltermodel.h"
#include "track.h"

#include <QDebug>

namespace PlaylistUi {
  ProxyFilterModel::ProxyFilterModel(QStyle *stl, const ColumnsConfig &col_cfg, ModusOperandi &modus, QObject *parent) : QSortFilterProxyModel(parent), modus_operandi(modus) {
    setFilterCaseSensitivity(Qt::CaseInsensitive);

    localfs = new Model(stl, col_cfg, this);
    connect(localfs, &Model::appendToPlaylistAsyncFinished, this, &ProxyFilterModel::appendToPlaylistAsyncFinished);
#ifdef ENABLE_MPD_SUPPORT
    mpd = new Mpd::Model(stl, col_cfg, modus.mpd_connection, this);
    connect(mpd, &Mpd::Model::appendToPlaylistAsyncFinished, this, &ProxyFilterModel::appendToPlaylistAsyncFinished);
    connect(&modus_operandi, &ModusOperandi::mpdReady, mpd, &Mpd::Model::reload);
    connect(&modus_operandi, &ModusOperandi::mpdLost, mpd, &Mpd::Model::onMpdLost);
#endif

    connect(&modus_operandi, &ModusOperandi::changed, this, &ProxyFilterModel::switchTo);
    switchTo(modus_operandi.get());
  }

  void ProxyFilterModel::switchTo(ModusOperandi::ActiveMode new_mode) {
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

  int ProxyFilterModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    if (activeModel()) {
      return activeModel()->rowCount();
    }
    return 0;
  }

  Model *ProxyFilterModel::activeModel() const {
    return qobject_cast<Model *>(sourceModel());
  }

  void ProxyFilterModel::filter(const QString &term) {
#if (QT_VERSION >= QT_VERSION_CHECK(6, 10, 0))
    beginFilterChange();
    filter_term = term;
    endFilterChange();
#else
    filter_term = term;
    invalidateFilter();
#endif
  }

  bool ProxyFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
    Q_UNUSED(source_parent)
    if (source_row >= activeModel()->rowCount()) {
      return true;
    }

    auto t = activeModel()->itemAt(activeModel()->buildIndex(source_row));

    return (t.artist().contains(filter_term, Qt::CaseInsensitive) ||
            t.album().contains(filter_term, Qt::CaseInsensitive) ||
            t.filename().contains(filter_term, Qt::CaseInsensitive) ||
            t.title().contains(filter_term, Qt::CaseInsensitive));
  }

  bool ProxyFilterModel::filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const {
    Q_UNUSED(source_parent)
    Q_UNUSED(source_column)
    return true;
  }
}
