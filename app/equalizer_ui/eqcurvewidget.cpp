#include "equalizer_ui/eqcurvewidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

#include <cmath>

namespace EqualizerUi {
  namespace {
    constexpr double kMinF = 20.0;
    constexpr double kMaxF = 20000.0;

    double freqToX(double f, int w) {
      return w * std::log10(f / kMinF) / std::log10(kMaxF / kMinF);
    }
  }

  CurveWidget::CurveWidget(QWidget *parent) : QWidget(parent) {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  }

  QSize CurveWidget::minimumSizeHint() const {
    const int line = fontMetrics().height();
    return QSize(line * 18, line * 7);
  }

  QSize CurveWidget::sizeHint() const {
    const int line = fontMetrics().height();
    return QSize(line * 32, line * 11);
  }

  void CurveWidget::setProfile(const Eq::EqProfile &profile, int fs) {
    eq_.setAutoPreamp(profile.auto_preamp);
    eq_.setBands(Eq::toStdBands(profile.bands));
    eq_.setPreampDb(profile.preamp_db);
    eq_.setSampleRate(fs > 0 ? fs : 48000);
    update();
  }

  void CurveWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const int w = width();
    const int h = height();
    const QPalette &pal = palette();
    const QColor text = pal.color(QPalette::Text);
    const QColor base = pal.color(QPalette::Base);
    const QColor grid = QColor(text.red(), text.green(), text.blue(), 40);

    p.fillRect(rect(), base);

    auto dbToY = [&](double db) {
      return h * (0.5 - db / (2.0 * range_db_));
    };

    p.setPen(grid);
    const double db_lines[] = {-12, -6, 0, 6, 12};
    for (double db : db_lines) {
      const int y = static_cast<int>(dbToY(db));
      QPen pen(db == 0.0 ? QColor(text.red(), text.green(), text.blue(), 90) : grid);
      p.setPen(pen);
      p.drawLine(0, y, w, y);
    }

    p.setPen(grid);
    const double f_lines[] = {50, 100, 200, 500, 1000, 2000, 5000, 10000};
    for (double f : f_lines) {
      const int x = static_cast<int>(freqToX(f, w));
      p.drawLine(x, 0, x, h);
    }

    QColor accent = pal.color(QPalette::Highlight);
    QPen curve_pen(accent);
    curve_pen.setWidth(2);
    p.setPen(curve_pen);

    QPainterPath path;
    const int steps = std::max(64, w);
    for (int i = 0; i <= steps; ++i) {
      const double t = static_cast<double>(i) / steps;
      const double f = kMinF * std::pow(kMaxF / kMinF, t);
      const double db = eq_.magnitudeResponseDb(f);
      const double x = t * w;
      const double y = dbToY(std::max(-range_db_, std::min(range_db_, db)));
      if (i == 0) {
        path.moveTo(x, y);
      } else {
        path.lineTo(x, y);
      }
    }
    p.drawPath(path);
  }
}
