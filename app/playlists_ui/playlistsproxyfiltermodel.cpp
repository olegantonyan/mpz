#include "playlistsproxyfiltermodel.h"

namespace PlaylistsUi {
  ProxyFilterModel::ProxyFilterModel(Model *source_model, QObject *parent) : QSortFilterProxyModel(parent) {
    Q_ASSERT(source_model);
    setSourceModel(source_model);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
  }

  int ProxyFilterModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    if (sourceModel()) {
      return sourceModel()->rowCount();
    }
    return 0;
  }
}
