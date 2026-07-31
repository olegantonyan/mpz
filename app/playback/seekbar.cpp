#include "playback/seekbar.h"

#include <QPainter>
#include <QPaintEvent>

namespace Playback {
  Seekbar::Seekbar(QWidget *parent) : QProgressBar(parent) {
    setTextVisible(false);
  }

  void Seekbar::setPeaks(const Waveform::Peaks &p) {
    peaks = p;
    buildColumns();
    update();
  }

  void Seekbar::setSlice(quint64 begin_ms, quint64 duration_ms) {
    if (slice_begin_ms == begin_ms && slice_duration_ms == duration_ms) {
      return;
    }
    slice_begin_ms = begin_ms;
    slice_duration_ms = duration_ms;
    buildColumns();
    update();
  }

  void Seekbar::resizeEvent(QResizeEvent *event) {
    QProgressBar::resizeEvent(event);
    buildColumns();
  }

  void Seekbar::buildColumns() {
    column_peak.clear();
    column_rms.clear();

    const int w = width();
    if (peaks.isEmpty() || w <= 0) {
      return;
    }

    const qint64 buckets = peaks.peak.size();
    const qint64 total_ms = qint64(peaks.duration_ms);
    const qint64 begin_ms = qMin(qint64(slice_begin_ms), total_ms);
    qint64 span_ms = slice_duration_ms > 0 ? qint64(slice_duration_ms) : total_ms - begin_ms;
    span_ms = qMin(span_ms, total_ms - begin_ms);
    if (span_ms <= 0) {
      return;
    }

    column_peak.resize(w);
    column_rms.resize(w);
    for (int x = 0; x < w; ++x) {
      const qint64 from_ms = begin_ms + span_ms * x / w;
      const qint64 to_ms = begin_ms + span_ms * (x + 1) / w;
      const qint64 from = from_ms / peaks.bucket_ms;
      const qint64 to = qMin(qMax(from + 1, to_ms / peaks.bucket_ms), buckets);

      quint8 hi = 0;
      quint8 mean = 0;
      for (qint64 i = from; i < to; ++i) {
        hi = qMax(hi, peaks.peak[i]);
        mean = qMax(mean, peaks.rms[i]);
      }
      column_peak[x] = hi;
      column_rms[x] = mean;
    }
  }

  void Seekbar::paintEvent(QPaintEvent *event) {
    if (column_peak.isEmpty() || column_peak.size() != width()) {
      QProgressBar::paintEvent(event);
      return;
    }

    const int w = width();
    const int h = height();
    const int mid = h / 2;
    const int half = qMax(1, h / 2 - 1);

    const QPalette &pal = palette();
    const QColor text = pal.color(QPalette::Text);
    const QColor highlight = pal.color(QPalette::Highlight);
    const QColor idle_peak(text.red(), text.green(), text.blue(), 60);
    const QColor idle_rms(text.red(), text.green(), text.blue(), 120);
    const QColor done_peak(highlight.red(), highlight.green(), highlight.blue(), 140);

    QPainter p(this);

    const qint64 span = qint64(maximum()) - minimum();
    const int played = span > 0 ? int(qint64(w) * (value() - minimum()) / span) : 0;
    for (int x = 0; x < w; ++x) {
      const bool done = x < played;
      p.setPen(done ? done_peak : idle_peak);
      const int ph = column_peak[x] * half / 255;
      p.drawLine(x, mid - ph, x, mid + ph);

      const int rh = column_rms[x] * half / 255;
      if (rh > 0) {
        p.setPen(done ? highlight : idle_rms);
        p.drawLine(x, mid - rh, x, mid + rh);
      }
    }

    if (played > 0 && played < w) {
      p.setPen(text);
      p.drawLine(played, 0, played, h - 1);
    }
  }
}
