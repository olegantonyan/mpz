#include "playlistproxyfiltermodel.h"
#include "track.h"

#include <QDebug>

namespace PlaylistUi {
  ProxyFilterModel::ProxyFilterModel(Model *model, QObject *parent) : QSortFilterProxyModel(parent), source_model(model) {
    Q_ASSERT(model);
    setSourceModel(model);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
  }

  int ProxyFilterModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    if (sourceModel()) {
      return sourceModel()->rowCount();
    }
    return 0;
  }

  void ProxyFilterModel::filter(const QString &term) {
    filter_term = term;
    invalidateFilter();
  }

  bool ProxyFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
    Q_UNUSED(source_parent)
    if (source_row >= source_model->rowCount()) {
      return true;
    }

    auto t = source_model->itemAt(source_model->buildIndex(source_row));

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
