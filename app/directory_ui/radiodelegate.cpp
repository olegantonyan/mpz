#include "radiodelegate.h"
#include "directorymodel/radio.h"

#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QStyle>

namespace {
  const int kPadding = 4;
  const int kGap = 8;

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

  // Stable per-station colour so each row stays recognisable without a logo.
  QColor badgeColor(const QString &seed) {
    const int hue = static_cast<int>(qHash(seed) % 360);
    return QColor::fromHsl(hue, 140, 130);
  }
}

namespace DirectoryUi {
  int RadioDelegate::badgeSize(const QStyleOptionViewItem &option) {
    return QFontMetrics(option.font).height() * 2;
  }

  QSize RadioDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
    if (!isStationRow(index)) {
      return QStyledItemDelegate::sizeHint(option, index);
    }

    const QSize base = QStyledItemDelegate::sizeHint(option, index);
    const int name_h = QFontMetrics(option.font).height();
    const int sub_h = QFontMetrics(subtitleFont(option.font)).height();
    const int content_h = qMax(badgeSize(option), name_h + sub_h + 2);
    return QSize(base.width(), qMax(base.height(), content_h + 2 * kPadding));
  }

  void RadioDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                            const QModelIndex &index) const {
    if (!isStationRow(index)) {
      QStyledItemDelegate::paint(painter, option, index);
      return;
    }

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    // Draw the platform's background/selection/focus chrome, but none of its
    // text or icon -- everything below is painted by hand.
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

    const int badge = badgeSize(opt);
    const QRect content = opt.rect.adjusted(kPadding, kPadding, -kPadding, -kPadding);
    const QRect badge_rect(content.left(), content.top() + (content.height() - badge) / 2,
                           badge, badge);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QString station_id = index.data(DirectoryModel::RadioRole::Path).toString();
    QPainterPath badge_path;
    badge_path.addRoundedRect(badge_rect, badge / 5.0, badge / 5.0);

    const QPixmap logo = index.data(DirectoryModel::RadioRole::Logo).value<QPixmap>();
    if (!logo.isNull()) {
      painter->save();
      painter->setClipPath(badge_path);
      painter->drawPixmap(badge_rect, logo.scaled(badge_rect.size(), Qt::KeepAspectRatioByExpanding,
                                                  Qt::SmoothTransformation));
      painter->restore();
    } else {
      // Colour badge stands in until the logo arrives, and forever offline.
      painter->fillPath(badge_path, badgeColor(station_id));
      if (!name.isEmpty()) {
        QFont badge_font = opt.font;
        badge_font.setBold(true);
        painter->setFont(badge_font);
        painter->setPen(opt.palette.color(QPalette::Normal, QPalette::HighlightedText));
        painter->drawText(badge_rect, Qt::AlignCenter, name.left(1).toUpper());
      }
    }

    const int text_left = badge_rect.right() + kGap;
    const int text_width = content.right() - text_left;
    if (text_width > 0) {
      const QFont sub_font = subtitleFont(opt.font);
      const QFontMetrics name_fm(opt.font);
      const QFontMetrics sub_fm(sub_font);
      const int block_h = name_fm.height() + sub_fm.height();
      int y = content.top() + (content.height() - block_h) / 2;

      painter->setFont(opt.font);
      painter->setPen(text_color);
      painter->drawText(QRect(text_left, y, text_width, name_fm.height()),
                        Qt::AlignLeft | Qt::AlignVCenter,
                        name_fm.elidedText(name, Qt::ElideRight, text_width));

      QString subtitle = index.data(DirectoryModel::RadioRole::Subtitle).toString();
      if (subtitle.isEmpty()) {
        subtitle = index.data(DirectoryModel::RadioRole::Description).toString();
      }
      if (!subtitle.isEmpty()) {
        QColor dim = text_color;
        dim.setAlphaF(0.65f);
        painter->setFont(sub_font);
        painter->setPen(dim);
        painter->drawText(QRect(text_left, y + name_fm.height(), text_width, sub_fm.height()),
                          Qt::AlignLeft | Qt::AlignVCenter,
                          sub_fm.elidedText(subtitle, Qt::ElideRight, text_width));
      }
    }

    painter->restore();
  }
}
