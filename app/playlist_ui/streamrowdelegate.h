#ifndef STREAMROWDELEGATE_H
#define STREAMROWDELEGATE_H

#include <QStyledItemDelegate>

namespace PlaylistUi {
  class StreamRowDelegate : public QStyledItemDelegate {
    Q_OBJECT

  public:
    explicit StreamRowDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
  };
}

#endif // STREAMROWDELEGATE_H
