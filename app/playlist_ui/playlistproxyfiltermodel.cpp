#include "playlistproxyfiltermodel.h"
#include "track.h"

#include <QDebug>

namespace PlaylistUi {
  ProxyFilterModel::ProxyFilterModel(QStyle *stl, const ColumnsConfig &col_cfg, QObject *parent) : QSortFilterProxyModel(parent) {
    setFilterCaseSensitivity(Qt::CaseInsensitive);

    localfs = new Model(stl, col_cfg, this);
    connect(localfs, &Model::appendToPlaylistAsyncFinished, this, &ProxyFilterModel::appendToPlaylistAsyncFinished);
    setSourceModel(localfs);
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
