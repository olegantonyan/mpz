#include "streamrowdelegate.h"
#include "playlistmodel.h"
#include "icons.h"

#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QStyle>

namespace {
  const int kPadding = 4;
  const int kGap = 6;
  const int kMaxIcon = 18;

  bool isStreamRow(const QModelIndex &index) {
    return index.data(PlaylistUi::Model::IsStreamRole).toBool();
  }
}

namespace PlaylistUi {
  StreamRowDelegate::StreamRowDelegate(QObject *parent) : QStyledItemDelegate(parent) {
  }

  void StreamRowDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    // Files, and column 0 of a stream row (the play/pause icon slot), render normally.
    if (index.column() == 0 || !isStreamRow(index)) {
      QStyledItemDelegate::paint(painter, option, index);
      return;
    }

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    // Keep the platform's selection/alternating-row chrome, paint content by hand.
    opt.text.clear();
    opt.icon = QIcon();
    opt.features &= ~QStyleOptionViewItem::HasDecoration;
    auto *style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

    const bool selected = opt.state & QStyle::State_Selected;
    const QPalette::ColorGroup group = (opt.state & QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled;
    const QColor text_color = opt.palette.color(group, selected ? QPalette::HighlightedText : QPalette::Text);

    QRect content = opt.rect.adjusted(kPadding, 0, -kPadding, 0);
    if (content.width() <= 0) {
      return;
    }

    painter->save();

    const int icon_sz = qBound(8, content.height() - 6, kMaxIcon);
    const QIcon::Mode mode = selected ? QIcon::Selected : QIcon::Normal;
    const QPixmap pm = Icons::pixmap(Icons::Icon::Radio, QSize(icon_sz, icon_sz), mode);
    const int icon_y = content.top() + (content.height() - icon_sz) / 2;
    painter->drawPixmap(QRect(content.left(), icon_y, icon_sz, icon_sz), pm);
    content.setLeft(content.left() + icon_sz + kGap);

    // Station name: bold on the highlighted (playing) row via the model's FontRole (opt.font).
    const QString station = index.data(Model::StationNameRole).toString();
    const QFontMetrics fm(opt.font);
    int x = content.left();
    int avail = content.right() - x + 1;
    if (avail > 0) {
      const QString elided = fm.elidedText(station, Qt::ElideRight, avail);
      painter->setFont(opt.font);
      painter->setPen(text_color);
      painter->drawText(QRect(x, content.top(), avail, content.height()), Qt::AlignLeft | Qt::AlignVCenter, elided);
      x += fm.horizontalAdvance(elided);
    }

    // Now-playing (live ICY metadata): dimmed, not bold, after a separator.
    const QString now = index.data(Model::StreamNowPlayingRole).toString();
    avail = content.right() - x + 1;
    if (!now.isEmpty() && avail > 0) {
      QFont now_font = opt.font;
      now_font.setBold(false);
      const QFontMetrics nfm(now_font);
      QColor dim = text_color;
      dim.setAlphaF(selected ? 0.8f : 0.6f);
      painter->setFont(now_font);
      painter->setPen(dim);
      const QString text = QStringLiteral("  —  ") + now;
      painter->drawText(QRect(x, content.top(), avail, content.height()), Qt::AlignLeft | Qt::AlignVCenter,
                        nfm.elidedText(text, Qt::ElideRight, avail));
    }

    painter->restore();
  }
}
