#include "radiodelegate.h"
#include "directorymodel/radio.h"

#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QStyle>

namespace {
  const int kPadding = 4;

  bool isStationRow(const QModelIndex &index) {
    return index.data(DirectoryUi::DirectoryModel::RadioRole::IsStation).toBool();
  }

  QFont subtitleFont(const QFont &base) {
    QFont f = base;
    if (f.pointSizeF() > 0) {
      f.setPointSizeF(f.pointSizeF() * 0.85);
    } else {
      f.setPixelSize(qMax(1, static_cast<int>(f.pixelSize() * 0.85)));
    }
    return f;
  }
}

namespace DirectoryUi {
  QSize RadioDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
    if (!isStationRow(index)) {
      return QStyledItemDelegate::sizeHint(option, index);
    }

    const QSize base = QStyledItemDelegate::sizeHint(option, index);
    const int name_h = QFontMetrics(option.font).height();
    const int sub_h = QFontMetrics(subtitleFont(option.font)).height();
    return QSize(base.width(), qMax(base.height(), name_h + sub_h + 2 + 2 * kPadding));
  }

  void RadioDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                            const QModelIndex &index) const {
    if (!isStationRow(index)) {
      QStyledItemDelegate::paint(painter, option, index);
      return;
    }

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    // Keep the platform's selection/focus chrome but paint the text by hand.
    const QString name = opt.text;
    opt.text.clear();
    opt.icon = QIcon();
    opt.features &= ~QStyleOptionViewItem::HasDecoration;
    auto *style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

    const bool selected = opt.state & QStyle::State_Selected;
    const QPalette::ColorGroup group = (opt.state & QStyle::State_Enabled)
                                         ? QPalette::Normal
                                         : QPalette::Disabled;
    const QColor text_color = opt.palette.color(
      group, selected ? QPalette::HighlightedText : QPalette::Text);

    const QRect content = opt.rect.adjusted(kPadding, kPadding, -kPadding, -kPadding);
    if (content.width() <= 0) {
      return;
    }

    const QFont sub_font = subtitleFont(opt.font);
    const QFontMetrics name_fm(opt.font);
    const QFontMetrics sub_fm(sub_font);
    int y = content.top() + (content.height() - name_fm.height() - sub_fm.height()) / 2;

    painter->save();
    painter->setFont(opt.font);
    painter->setPen(text_color);
    painter->drawText(QRect(content.left(), y, content.width(), name_fm.height()),
                      Qt::AlignLeft | Qt::AlignVCenter,
                      name_fm.elidedText(name, Qt::ElideRight, content.width()));

    const QString subtitle = index.data(DirectoryModel::RadioRole::Subtitle).toString();
    if (!subtitle.isEmpty()) {
      QColor dim = text_color;
      dim.setAlphaF(0.65f);
      painter->setFont(sub_font);
      painter->setPen(dim);
      painter->drawText(QRect(content.left(), y + name_fm.height(), content.width(), sub_fm.height()),
                        Qt::AlignLeft | Qt::AlignVCenter,
                        sub_fm.elidedText(subtitle, Qt::ElideRight, content.width()));
    }
    painter->restore();
  }
}
