#ifndef RADIODELEGATE_H
#define RADIODELEGATE_H

#include <QStyledItemDelegate>

namespace DirectoryUi {
  // Rows that don't answer RadioRole::IsStation fall through to the base
  // delegate, so this is safe to install on the library tree unconditionally.
  class RadioDelegate : public QStyledItemDelegate {
    Q_OBJECT

  public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
  };
}

#endif // RADIODELEGATE_H
