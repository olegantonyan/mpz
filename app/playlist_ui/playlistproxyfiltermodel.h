#ifndef PLAYLISTPROXYFILTERMODEL_H
#define PLAYLISTPROXYFILTERMODEL_H

#include "playlistmodel.h"

#include <QObject>
#include <QSortFilterProxyModel>

namespace PlaylistUi {
  class ProxyFilterModel : public QSortFilterProxyModel {
    Q_OBJECT
  public:
    explicit ProxyFilterModel(Model *model, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

  public slots:
    void filter(const QString &term);

  protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    bool filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const override;

  private:
    Model *source_model;
    QString filter_term;
  };
}

#endif // PLAYLISTPROXYFILTERMODEL_H
