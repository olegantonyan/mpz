#ifndef SEEKBAR_H
#define SEEKBAR_H

#include "waveform/peaks.h"

#include <QProgressBar>
#include <QVector>

namespace Playback {
  class Seekbar : public QProgressBar {
    Q_OBJECT

  public:
    explicit Seekbar(QWidget *parent = nullptr);

    void setPeaks(const Waveform::Peaks &peaks);
    void setSlice(quint64 begin_ms, quint64 duration_ms);

  protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

  private:
    void buildColumns();

    Waveform::Peaks peaks;
    QVector<quint8> column_peak;
    QVector<quint8> column_rms;
    quint64 slice_begin_ms = 0;
    quint64 slice_duration_ms = 0;
  };
}

#endif // SEEKBAR_H
