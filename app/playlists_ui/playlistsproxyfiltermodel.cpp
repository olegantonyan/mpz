#include "playlistsproxyfiltermodel.h"

namespace PlaylistsUi {
  ProxyFilterModel::ProxyFilterModel(Config::Local &conf, ModusOperandi &modus, QObject *parent) : QSortFilterProxyModel(parent), modus_operandi(modus) {
    setFilterCaseSensitivity(Qt::CaseInsensitive);

    localfs = new PlaylistsUi::Model(conf, this);
    connect(localfs, &Model::asyncLoadFinished, this, &ProxyFilterModel::asyncLoadFinished);
    connect(localfs, &Model::createPlaylistAsyncFinished, this, &ProxyFilterModel::createPlaylistAsyncFinished);
#ifdef ENABLE_MPD_SUPPORT
    mpd = new Mpd::Model(conf, modus.mpd_connection, this);
    connect(mpd, &Mpd::Model::asyncLoadFinished, this, &ProxyFilterModel::asyncLoadFinished);
    connect(mpd, &Mpd::Model::createPlaylistAsyncFinished, this, &ProxyFilterModel::createPlaylistAsyncFinished);
#endif
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
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    if(!filterRegularExpression().pattern().isEmpty()) {
      return QSortFilterProxyModel::rowCount(parent);
    }
#else
    if(!filterRegExp().isEmpty()) {
      return QSortFilterProxyModel::rowCount(parent);
    }
#endif
    if (activeModel()) {
      return activeModel()->rowCount();
    }
    return 0;
  }

  std::shared_ptr<Playlist::Playlist> ProxyFilterModel::itemAt(const QModelIndex &index) const {
    return activeModel()->itemAt(mapToSource(index));
  }

  bool ProxyFilterModel::persist() {
    return activeModel()->persist();
  }

  QModelIndex ProxyFilterModel::append(std::shared_ptr<Playlist::Playlist> pl) {
    return mapFromSource(activeModel()->append(pl));
  }

  Model *ProxyFilterModel::activeModel() const {
    switch (modus_operandi.get()) {
    case ModusOperandi::MODUS_MPD:
#ifdef ENABLE_MPD_SUPPORT
      return qobject_cast<Mpd::Model *>(sourceModel());
#endif
    case ModusOperandi::MODUS_LOCALFS:
    default:
      return qobject_cast<Model *>(sourceModel());
    }
  }
}
