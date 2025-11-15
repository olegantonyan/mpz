#include "playlistsproxyfiltermodel.h"

namespace PlaylistsUi {
  ProxyFilterModel::ProxyFilterModel(Config::Local &conf, QObject *parent) : QSortFilterProxyModel(parent) {
    auto source_model = new PlaylistsUi::Model(conf, this);
    setSourceModel(source_model);

    connect(source_model, &Model::asyncLoadFinished, this, &ProxyFilterModel::asyncLoadFinished);
    connect(source_model, &Model::createPlaylistAsyncFinished, this, &ProxyFilterModel::createPlaylistAsyncFinished);

    setFilterCaseSensitivity(Qt::CaseInsensitive);
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
    if (sourceModel()) {
      return sourceModel()->rowCount();
    }
    return 0;
  }

  std::shared_ptr<Playlist::Playlist> ProxyFilterModel::itemAt(const QModelIndex &index) const {
    Model *s = qobject_cast<Model *>(sourceModel());
    return s->itemAt(mapToSource(index));
  }

  bool ProxyFilterModel::persist() {
    return activeModel()->persist();
  }

  QModelIndex ProxyFilterModel::append(std::shared_ptr<Playlist::Playlist> pl) {
    return mapFromSource(activeModel()->append(pl));
  }

  Model *ProxyFilterModel::activeModel() const {
    Model *s = qobject_cast<Model *>(sourceModel());
    return s;
  }
}
