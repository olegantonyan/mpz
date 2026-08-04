#include "floatingheader.h"

#include <QAbstractItemModel>
#include <QCursor>
#include <QHeaderView>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionHeader>

namespace {
  const qreal kMaxOpacity = 0.85;
  const int kFadeInMs = 120;
  const int kFadeOutMs = 200;
  const int kHideGraceMs = 350;
  const int kRevealSlack = 6;
  const int kKeepSlack = 28;

  bool same(qreal a, qreal b) {
    return qAbs(a - b) < 0.001;
  }
}

namespace PlaylistUi {
  // Parented to the view rather than to its viewport: QWidget::scroll() physically
  // moves every child, so a viewport child would slide away with the rows.
  FloatingHeader::FloatingHeader(QTableView *v) : QWidget(v), view(v) {
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setFocusPolicy(Qt::NoFocus);
    setAcceptDrops(false);
    hide();

    animation = new QPropertyAnimation(this, "opacity", this);
    connect(animation, &QPropertyAnimation::finished, this, [this]() {
      if (opacity_value <= 0.001) {
        QWidget::hide();
      }
    });

    hide_timer.setSingleShot(true);
    connect(&hide_timer, &QTimer::timeout, this, &FloatingHeader::hideNow);
  }

  void FloatingHeader::setColumns(const QStringList &l, const QVector<Qt::Alignment> &a) {
    labels = l;
    aligns = a;
    invalidateCache();
  }

  void FloatingHeader::setActive(bool on) {
    active = on;
    if (!active) {
      hideNow();
    }
  }

  qreal FloatingHeader::opacity() const {
    return opacity_value;
  }

  void FloatingHeader::setOpacity(qreal v) {
    opacity_value = v;
    update();
  }

  int FloatingHeader::preferredHeight() const {
    const int h = view->horizontalHeader()->sizeHint().height();
    return h > 0 ? h : fontMetrics().height() + 6;
  }

  bool FloatingHeader::inRevealZone(int y, int header_height, bool visible) {
    if (y < 0) {
      return false;
    }
    return y <= header_height + (visible ? kKeepSlack : kRevealSlack);
  }

  void FloatingHeader::syncGeometry() {
    const QWidget *vp = view->viewport();
    setGeometry(vp->x(), vp->y(), vp->width(), preferredHeight());
    invalidateCache();
  }

  void FloatingHeader::invalidateCache() {
    cache = QPixmap();
    update();
  }

  void FloatingHeader::reveal() {
    hide_timer.stop();
    if (!active || suppressed || labels.isEmpty()) {
      return;
    }
    if (!isVisible()) {
      syncGeometry();
      raise();
      show();
    }
    animateTo(kMaxOpacity, kFadeInMs);
  }

  void FloatingHeader::scheduleHide() {
    if (isVisible() && !hide_timer.isActive()) {
      hide_timer.start(kHideGraceMs);
    }
  }

  void FloatingHeader::hideNow() {
    hide_timer.stop();
    if (isVisible()) {
      animateTo(0.0, kFadeOutMs);
    }
  }

  void FloatingHeader::setSuppressed(bool on) {
    suppressed = on;
    if (on) {
      hideNow();
    }
  }

  void FloatingHeader::onViewportEvent(QEvent *event) {
    switch (event->type()) {
    case QEvent::Resize:
      syncGeometry();
      break;
    case QEvent::Enter:
      trackPointer(view->viewport()->mapFromGlobal(QCursor::pos()).y());
      break;
    case QEvent::MouseMove:
      trackPointer(static_cast<QMouseEvent *>(event)->pos().y());
      break;
    case QEvent::Leave:
    case QEvent::WindowDeactivate:
      hideNow();
      break;
    case QEvent::DragEnter:
    case QEvent::DragMove:
      setSuppressed(true);
      break;
    case QEvent::DragLeave:
    case QEvent::Drop:
      setSuppressed(false);
      break;
    case QEvent::Wheel:
      if (isVisible()) {
        update();
      }
      break;
    default:
      break;
    }
  }

  void FloatingHeader::onViewEvent(QEvent *event) {
    if (event->type() == QEvent::KeyPress || event->type() == QEvent::FocusOut) {
      hideNow();
    }
  }

  void FloatingHeader::trackPointer(int y) {
    if (inRevealZone(y, preferredHeight(), isVisible())) {
      reveal();
    } else {
      scheduleHide();
    }
  }

  void FloatingHeader::animateTo(qreal target, int duration_ms) {
    const bool running = animation->state() == QAbstractAnimation::Running;
    if (same(running ? animation->endValue().toReal() : opacity_value, target)) {
      return;
    }
    animation->stop();
    animation->setDuration(duration_ms);
    animation->setStartValue(opacity_value);
    animation->setEndValue(target);
    animation->start();
  }

  void FloatingHeader::paintEvent(QPaintEvent *) {
    if (opacity_value <= 0.001 || width() <= 0 || height() <= 0) {
      return;
    }
    if (cache.isNull() || !qFuzzyCompare(cache.devicePixelRatio(), devicePixelRatioF())) {
      renderCache();
    }
    QPainter painter(this);
    painter.setOpacity(opacity_value);
    painter.drawPixmap(0, 0, cache);
  }

  void FloatingHeader::resizeEvent(QResizeEvent *) {
    invalidateCache();
  }

  void FloatingHeader::changeEvent(QEvent *event) {
    QWidget::changeEvent(event);
    const auto type = event->type();
    if (type == QEvent::PaletteChange || type == QEvent::FontChange || type == QEvent::StyleChange) {
      invalidateCache();
    }
  }

  // Section geometry is read from the view on every repaint instead of mirrored:
  // Stretch columns settle on a delayed timer, so any snapshot is a frame stale.
  void FloatingHeader::renderCache() {
    const qreal dpr = devicePixelRatioF();
    cache = QPixmap(size() * dpr);
    cache.setDevicePixelRatio(dpr);
    cache.fill(Qt::transparent);

    const int cols = view->model() != nullptr ? view->model()->columnCount() : 0;
    QPainter p(&cache);

    for (int c = 0; c < cols; c++) {
      const int x = view->columnViewportPosition(c);
      const int w = view->columnWidth(c);
      if (w <= 0 || x >= width() || x + w <= 0) {
        continue;
      }

      QStyleOptionHeader opt;
      opt.initFrom(this);
      opt.state |= QStyle::State_Horizontal | QStyle::State_Raised;
      opt.state &= ~(QStyle::State_MouseOver | QStyle::State_Sunken | QStyle::State_HasFocus);
      opt.orientation = Qt::Horizontal;
      opt.section = c;
      opt.rect = QRect(x, 0, w, height());
      opt.sortIndicator = QStyleOptionHeader::None;
      opt.selectedPosition = QStyleOptionHeader::NotAdjacent;
      opt.position = cols == 1 ? QStyleOptionHeader::OnlyOneSection
                   : c == 0 ? QStyleOptionHeader::Beginning
                   : c == cols - 1 ? QStyleOptionHeader::End
                   : QStyleOptionHeader::Middle;
      opt.textAlignment = c < aligns.size() ? aligns.at(c) : (Qt::AlignLeft | Qt::AlignVCenter);

      const QString raw = c < labels.size() ? labels.at(c) : QString();
      const int text_width = w - 2 * style()->pixelMetric(QStyle::PM_HeaderMargin, &opt, this);
      if (!raw.isEmpty() && text_width > 0) {
        opt.text = fontMetrics().elidedText(raw, Qt::ElideRight, text_width);
      }

      style()->drawControl(QStyle::CE_Header, &opt, &p, this);
    }

    p.setPen(palette().color(QPalette::Mid));
    p.drawLine(0, height() - 1, width(), height() - 1);
  }
}
