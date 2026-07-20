#ifndef RADIODELEGATE_H
#define RADIODELEGATE_H

#include <QStyledItemDelegate>

namespace DirectoryUi {
  // Paints radio station rows as two lines (name + codec/bitrate/description)
  // with a colour badge for the station. Installed unconditionally on the
  // library tree: rows that don't answer RadioRole::IsStation -- every localfs
  // and mpd row, and radio group rows -- fall through to the base delegate.
  class RadioDelegate : public QStyledItemDelegate {
    Q_OBJECT

  public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

  private:
    static int badgeSize(const QStyleOptionViewItem &option);
  };
}

#endif // RADIODELEGATE_H
