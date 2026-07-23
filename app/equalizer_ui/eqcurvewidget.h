#ifndef EQ_CURVE_WIDGET_H
#define EQ_CURVE_WIDGET_H

#include "eq/equalizer.h"
#include "eq/eqprofile.h"

#include <QWidget>

namespace EqualizerUi {
  class CurveWidget : public QWidget {
    Q_OBJECT
  public:
    explicit CurveWidget(QWidget *parent = nullptr);

    void setProfile(const Eq::EqProfile &profile, int fs);
    double autoPreampDb() const { return eq_.autoPreampDb(); }

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

  protected:
    void paintEvent(QPaintEvent *event) override;

  private:
    Eq::Equalizer eq_;
    double range_db_ = 15.0;
  };
}

#endif // EQ_CURVE_WIDGET_H
