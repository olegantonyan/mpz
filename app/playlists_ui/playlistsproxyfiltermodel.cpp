#include "playlistsproxyfiltermodel.h"

namespace PlaylistsUi {
  ProxyFilterModel::ProxyFilterModel(Model *source_model, QObject *parent) : QSortFilterProxyModel(parent) {
    Q_ASSERT(source_model);
    setSourceModel(source_model);
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
    Model *s = qobject_cast<Model *>(sourceModel());
    return s->persist();
  }
}
