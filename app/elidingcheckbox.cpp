#include "elidingcheckbox.h"

#include <QStyle>
#include <QStyleOptionButton>
#include <QStylePainter>

namespace {
  constexpr int kMinTextChars = 4;
}

// QCheckBox defaults to QSizePolicy::Minimum, which has no ShrinkFlag, so layouts ignore
// minimumSizeHint() and pin the full sizeHint() width instead.
ElidingCheckBox::ElidingCheckBox(QWidget *parent) : QCheckBox(parent) {
  setSizePolicy(QSizePolicy::Preferred, sizePolicy().verticalPolicy());
}

QSize ElidingCheckBox::minimumSizeHint() const {
  QStyleOptionButton opt;
  initStyleOption(&opt);
  const int indicator = style()->pixelMetric(QStyle::PM_IndicatorWidth, &opt, this)
                      + style()->pixelMetric(QStyle::PM_CheckBoxLabelSpacing, &opt, this);
  return QSize(indicator + fontMetrics().averageCharWidth() * kMinTextChars, QCheckBox::minimumSizeHint().height());
}

void ElidingCheckBox::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event)
  QStylePainter painter(this);
  QStyleOptionButton opt;
  initStyleOption(&opt);
  const int available = style()->subElementRect(QStyle::SE_CheckBoxContents, &opt, this).width();
  opt.text = fontMetrics().elidedText(text(), Qt::ElideRight, available);
  painter.drawControl(QStyle::CE_CheckBox, opt);
}
