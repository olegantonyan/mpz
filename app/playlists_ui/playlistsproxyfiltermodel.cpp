#include "playlistsproxyfiltermodel.h"

namespace PlaylistsUi {
  ProxyFilterModel::ProxyFilterModel(QObject *parent) : QSortFilterProxyModel(parent) {
    setSourceModel(nullptr);
  }

  int ProxyFilterModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    if (sourceModel()) {
      return sourceModel()->rowCount();
    }
    return 0;
  }
}
