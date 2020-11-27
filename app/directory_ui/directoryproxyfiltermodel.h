#ifndef DIRECTORYPROXYFILTERMODEL_H
#define DIRECTORYPROXYFILTERMODEL_H

#include "directory_ui/directorymodel.h"

#include <QObject>
#include <QSortFilterProxyModel>

namespace DirectoryUi {
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

#endif // DIRECTORYPROXYFILTERMODEL_H
