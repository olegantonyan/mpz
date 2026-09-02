#ifndef ELIDINGCHECKBOX_H
#define ELIDINGCHECKBOX_H

#include <QCheckBox>
#include <QPaintEvent>
#include <QSize>
#include <QWidget>

class ElidingCheckBox : public QCheckBox {
  Q_OBJECT
public:
  explicit ElidingCheckBox(QWidget *parent = nullptr);

  QSize minimumSizeHint() const override;

protected:
  void paintEvent(QPaintEvent *event) override;
};

#endif // ELIDINGCHECKBOX_H
